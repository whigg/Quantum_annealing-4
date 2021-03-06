// the code with proper boundaries between the domains
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <mpi.h>	
#include <sys/time.h>

using namespace std;

// timing functions
double get_wall_time(){
    struct timeval time;
    if (gettimeofday(&time,NULL)){
        return 0;}
    return (double)time.tv_sec + (double)time.tv_usec * .000001;}
double get_cpu_time(){
    return (double)clock() / CLOCKS_PER_SEC;}

int main(int argc, char *argv[]){
  //  Start Timers
  double wall0 = get_wall_time();
  double cpu0  = get_cpu_time();

  double J=+1,H=0,T,k=1;
  int m=200, n=200;
  int id, ntasks;
  int total_steps=5100;  int therm_steps=int(0.2*total_steps);
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &ntasks);
  MPI_Comm_rank(MPI_COMM_WORLD, &id);

// Memory and variable allocation stage
////////////////////////////////////////////////////////////////////////////////////////////////////////

  if(id==0){ofstream file("ising.txt");file.close();ofstream mfile("magnetization.txt");mfile.close();}

 // changing value of n to make sure that each process recives the same ammount of data 
  int RowsPerTask = ceil(double(n)/double(ntasks));
  n=RowsPerTask*ntasks;

 // now we need to simulate periodic boundary by additional data columns for each process
 // int M=m+2; // WARNING NOTATION two additional rows, one at the top and one at the bottom
 int N=n+ntasks*2; // two extra column for each data band

 // 1D array to collect magnetization data in root process
 double* collect_data = new double [ntasks];
 
 // 1D spin orientation data recieved by single array
 double* s_recv = new double[(m+2)*(RowsPerTask+2)];
 // 2D array of spins for indiviudal process for its own chank of data
 double* spin_s = new double[(m+2)*(RowsPerTask+2)];
 double** s_pins = new double*[m+2];
 // initialize it
 for(int i = 0; i < m+2; ++i)
    s_pins[i] = spin_s + (RowsPerTask+2)*i;

 // create collective 2D array of spins
 double* s_data = new double[m*n];
 double** s = new double*[m];
 // initialize it
 for(int i = 0; i < m; i++)
    s[i] = s_data + n*i;
 // set element values =1
 int x0=round(m/2); int y0=round(m/2);
 for(int j=0;j<n;j++){for(int i=0;i<m;i++){if((x0-i)*(x0-i)+(y0-j)*(y0-j)<=60){s[i][j] = -1;}else{s[i][j] = +1;}}}

 // create linear array of spins to distribute data between the processes 
 double* s_send = new double[(m+2)*N];

 // Array will be holding all data
  double* S_data = new double[(m+2)*N];
  double** S = new double*[(m+2)];
  for (int i = 0; i < (m+2); ++i)
   S[i] = S_data + N*i;
    for(int i=0;i<(m+2);i++){for(int j=0;j<N;j++){S[i][j] = 0;}}

//************************
//************************
//************************
 double dT=0.01; srand ((double(id)+1)*time(NULL));
  for(double T=dT;T<=3;T+=dT){
      double M=0;
      for(int c_ount=1;c_ount<=total_steps;c_ount++){

if(id==0){
// Active part of DATA reshafling  
//*//*//*//*//*//*//*//*//*//*//*//*//*//*//*//*//*//*//*//*//*//*//*//*//*//*//*//*//*

  int cou_nt=0;
  for(int p_os=1;p_os<=1+(ntasks-1)*(RowsPerTask+2);p_os+=(RowsPerTask+2)){
   for(int j=0+p_os;j<RowsPerTask+p_os;j++){
    for(int i=1;i<(m+2)-1;i++){
      S[i][j] = s[i-1][j-1-2*cou_nt];}}cou_nt++;}

 // global top and bottom boundaries
  for(int j=0;j<N;j++){S[0][j]=S[(m+2)-2][j];S[(m+2)-1][j]=S[1][j];}
 // global left and right boundaries
  for(int i=0;i<(m+2);i++){S[i][0]=S[i][N-2];S[i][N-1]=S[i][1];}
 // internal boundaries
  for(int j=0;j<ntasks-1;j++){
   for(int i=0;i<(m+2);i++){
	S[i][(j+1)*(RowsPerTask+1)+j]=S[i][(j+1)*(RowsPerTask+1)+j+2];
	S[i][(j+1)*(RowsPerTask+1)+j+1]=S[i][(j+1)*(RowsPerTask+1)+j-1];}}

  // reassigning 2D data to this 1D array for further distribution
  // COLUMN by COLUMN
   for (int t = 0; t < N; t++){
    for (int q = 0; q < (m+2); q++){
     s_send[t * (m+2) + q] = S[q][t];}}
}
  // distribute data
  // individual process gets its data beyond this point
 	MPI_Scatter(s_send,(m+2)*(RowsPerTask+2),MPI_DOUBLE,
			s_recv,(m+2)*(RowsPerTask+2),MPI_DOUBLE,0,MPI_COMM_WORLD);

   // reconstruct from linear to 2D
   for (int t = 0; t < RowsPerTask+2; t++){
    for (int q = 0; q < m+2; q++){
     s_pins[q][t] = s_recv[t * (m+2) + q];}}

//  double dT=0.02; srand ((double(id)+1)*time(NULL));
//  for(double T=dT;T<=3;T+=dT){
//      double M=0;
//      for(int c_ount=1;c_ount<=total_steps;c_ount++){
        for (int ii=0;ii<m*RowsPerTask;ii++){
	    int i = rand()%m+1; int j = rand()%RowsPerTask+1; 
            double dE=2*J*s_pins[i][j]*(s_pins[i+1][j] + s_pins[i-1][j] + s_pins[i][j-1] + s_pins[i][j+1]);
            if (exp(-dE/(k*T))>(rand()/(RAND_MAX + 1.0))){s_pins[i][j] = -s_pins[i][j];}
         }

    double M_0=0;
    for(int i=0;i<m;i++){for(int j=0;j<RowsPerTask;j++){M_0+=s_pins[i][j];}}
     if (c_ount>=therm_steps){M+=M_0;}
 //   } // loop over mont carlo steps
 

 //    cout<<id<<"\t"<<T<<"\t"<<M/double(m*RowsPerTask)/double(0.8*total_steps)<<"\n"; //?? cast double to int

   //  double M_local=M/double(m*RowsPerTask)/double(0.8*total_steps);
   //  MPI_Gather(&M_local,1,MPI_DOUBLE,
 //	collect_data,1,MPI_DOUBLE,0,MPI_COMM_WORLD);
   
     // convert from 2D to linear to send back
     for(int t=0; t<(RowsPerTask+2); t++){
        for(int q = 0; q<(m+2); q++){
           s_recv[t*(m+2)+q] = s_pins[q][t];
      }}

     // send it back to root process
    MPI_Gather(s_recv,(m+2)*(RowsPerTask+2),MPI_DOUBLE,
 	s_send,(m+2)*(RowsPerTask+2),MPI_DOUBLE,0,MPI_COMM_WORLD);

    if(id==0){
  //    ofstream file("ising.txt",std::ofstream::out | std::ofstream::app);
   //   double M_total=0;
   //   for(int i=0;i<ntasks;i++)
   //   M_total+=collect_data[i];
   //   file<<T<<"\t"<<M_total/ntasks<<"\n";file.close();

 //     ofstream mfile("magnetization.txt",std::ofstream::out | std::ofstream::app); 
 
     // reconstruction from 1D
     for(int t=0;t<N;t++){
        for(int q=0;q<(m+2);q++){
           S[q][t]= s_send[t*(m+2)+q];}}

     // back reconstruction of the expanded array to the original ones
     int cou_nt=0;
     for(int p_os=1;p_os<=1+(ntasks-1)*(RowsPerTask+2);p_os+=(RowsPerTask+2)){
        for(int j=0+p_os;j<RowsPerTask+p_os;j++){
           for(int i=1;i<(m+2)-1;i++){
              s[i-1][j-1-2*cou_nt]=S[i][j];
           }}cou_nt++;} 

//     for(int q = 0; q < m; q++){
  //      for(int t = 0; t < n; t++){
   //        mfile<<s[q][t]<<"\t";}mfile<<"\n";}mfile.close();

} // loop over id==0

} // loop over monte carlo steps
     double M_local=M/double(m*RowsPerTask)/double(0.8*total_steps);
     MPI_Gather(&M_local,1,MPI_DOUBLE,
 	collect_data,1,MPI_DOUBLE,0,MPI_COMM_WORLD);

 if(id==0){
      cout<<id<<"\t"<<T<<"\t"<<M/double(m*RowsPerTask)/double(0.8*total_steps)<<"\n";
      ofstream file("ising.txt",std::ofstream::out | std::ofstream::app);
      double M_total=0;
      for(int i=0;i<ntasks;i++)
      M_total+=collect_data[i];
      file<<T<<"\t"<<M_total/ntasks<<"\n";file.close();

  ofstream mfile("magnetization.txt",std::ofstream::out | std::ofstream::app); 
     for(int q = 0; q < m; q++){
        for(int t = 0; t < n; t++){
           mfile<<s[q][t]<<"\t";}mfile<<"\n";}mfile.close();
}


} // loop over T
//************************
//************************
//************************


delete[] spin_s;
delete[] s_pins;
delete[] s_recv;

delete[] s_data;
delete[] s;
delete[] s_send;



delete[] S_data;
delete[] S;

delete[] collect_data;

MPI_Finalize();
    //  Stop timers
    double wall1 = get_wall_time();
    double cpu1  = get_cpu_time();
cout<<wall1-wall0<<"\t"<<cpu1-cpu0<<"\n";
  return 0;
}
