#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <mpi.h>

using namespace std;

const double pi = 3.14159;
const int qq=2;
const int m=32, n=32;
int nn;

// calculate energy for spin grid with extra edges
double e_nergy(int *S[], double* cos_theta[], int m, int n, double J){
    double s_um=0;
    for(int i=1; i<m-1; i++){
        for(int j=1; j<n-1; j++){
	s_um=s_um+(cos_theta[S[i][j]][S[i+1][j]]+
			   cos_theta[S[i][j]][S[i-1][j]]+
			   cos_theta[S[i][j]][S[i][j+1]]+
			   cos_theta[S[i][j]][S[i][j-1]]);}}
    return s_um/2*J;}

int main(int argc, char *argv[]){
	int id, ntasks;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &ntasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &id);

	//cout<<setprecision(3);
	double r_atio=0.1;
	double J=-1,E1t,E2t;
	double E_min=-2.0+double(id)*4/ntasks, E_max=-2.0+double(id+1)*4/ntasks;
	cout<<"\n\n\n\n"<<id<<"\t"<<E_min<<"\t"<<E_max<<"\n\n\n\n";
	int l_ock=0;
	long int total_steps=90000000;
	
	double f=2.71828;
	double* F = new double[1];
	F[0]=f;
	// Introduce arrays to keep log_gE and H_E data
	int n_odes=m*n*4+1;
	double* log_gE = new double[n_odes];
	double* log_gE_total = new double[n_odes*ntasks];
	double* H_E    = new double[n_odes];
	double* H_E_total = new double[n_odes*ntasks];

	double* EpN = new double[n_odes];
	for(int i=0;i<n_odes;i++){
		log_gE[i]=0;H_E[i]=0;EpN[i]=double(i-2*m*n);}
		
	// create the main, basic 2D array of spins without extra edges
	int* s_data = new int[m*n];
	int** s = new int*[m];
	// initialize it
	for(int i = 0; i < m; i++){
		s[i] = s_data + n*i;}
	
	// see WikiPedia for Potts model description
	// set element values such that cos(2*pi*s[i][j]/qq) = -1 for qq=2 ==> cos(pi), for qq>2 see by yourself..
	for(int j=0;j<n;j++){
		for(int i=0;i<m;i++){
			s[i][j] = qq-1;}}
			
	//  This array will be holding all data including the boundaries
	int* S_data = new int[(m+2)*(n+2)];
	int** S = new int*[(m+2)];
	for(int i = 0; i < (m+2); ++i){
		S[i] = S_data + (n+2)*i;}
	for(int i=1;i<(m+1);i++){
		for(int j=1;j<(n+1);j++){
			S[i][j] = s[i-1][j-1];}}
	
	// do I really need the next line to initialize the corner elements?
	S[0][0]=0;S[m+1][n+1]=0;S[0][n+1]=0;S[m+1][0]=0;
	// global top and bottom boundaries
	for(int j=0;j<(n+2);j++){S[0][j]=S[m][j];S[m+1][j]=S[1][j];}
	// global left and right boundaries
	for(int i=0;i<(m+2);i++){S[i][0]=S[i][n];S[i][n+1]=S[i][1];}
	// NEED TO VISUALLY CHECK THE PREVIOUS STEPS
	
	// 2D array of the precomputed cos(theta_j-theta_i) values
	// makes program a little bit faster
	double* cost_heta = new double[qq*qq];
	double** cos_theta = new double*[qq];
	// initialize it
	for(int i = 0; i < qq; ++i){
		cos_theta[i] = cost_heta + qq*i;}
	// precomputing the array
	for(int t = 0; t < qq; t++){
		for (int q = 0; q < qq; q++){
			cos_theta[q][t] = cos(2*pi*double(q-t)/qq);}} // do I need two doubles?
			
	// energy of the initial configuration of spins on the grid => E1t
	E1t=e_nergy(S, cos_theta, m+2, n+2, J);

	int c_ount=0;
	while(c_ount<=total_steps){c_ount++;
	 //while(c_ount<=0){c_ount++;E2t=0;
		for (int ii=0;ii<1000*m*n;ii++){ // we assume that we should randomly access pretty much all spins in the array
		double E1t_old=E1t;
			int i = rand()%m+1; int j = rand()%n+1; // the indices should fall within the boundaries
			int nn =rand()%qq; // nn takes values from 0 to qq-1, because array indexed from 0 ==> cos(2pi*nn/qq)....
			int nn_old=S[i][j];S[i][j]=nn;
			// update boundaries in order to use functions e_nergy and d_E!!
			if((i==1)||(i==m)||(j==1)||(j==n)){// update the big matrix boundaries
			 // top and bottom boundaries update
			 for(int j=0;j<(n+2);j++){S[0][j]=S[m][j];S[m+1][j]=S[1][j];}
			 // left and right boundaries update
			 for(int i=0;i<(m+2);i++){S[i][0]=S[i][n];S[i][n+1]=S[i][1];}}
									
			E2t=e_nergy(S, cos_theta, m+2, n+2, J);

double old_lock=l_ock;
//cout<<l_ock<<"\t"<<old_lock<<"\t"<<E_min<<"\t"<<E2t/(m*n)<<"\t"<<E_max<<"\n";
if((E_min<=E2t/(m*n)) && (E2t/(m*n)<=E_max)){l_ock=1;}
else{l_ock=0;} // scanning from -2 towards +2 ... We need to reach the proper energy spot and avoid pitfalls?
			if (exp(log_gE[int(E1t)+2*m*n]-log_gE[int(E2t)+2*m*n])>=rand()/(RAND_MAX + 1.0)){
				E1t=E2t;
if(l_ock==1){			log_gE[int(E2t)+2*m*n]+=log(f);
				H_E[int(E2t)+2*m*n]+=1;}
				s[i-1][j-1] = nn; // update original small matrix
if((l_ock==0)&&(old_lock==1)){s[i-1][j-1]=nn_old;S[i][j]=nn_old; E1t=E1t_old; l_ock=1;// force walker to come back FROM BOTH SIDES
				// update boundaries back
				if((i==1)||(i==m)||(j==1)||(j==n)){// update the big matrix boundaries
				// top and bottom boundaries update
				for(int j=0;j<(n+2);j++){S[0][j]=S[m][j];S[m+1][j]=S[1][j];}
				// left and right boundaries update
				for(int i=0;i<(m+2);i++){S[i][0]=S[i][n];S[i][n+1]=S[i][1];}}}
			}else{
if(l_ock==1){			log_gE[int(E1t)+2*m*n]+=log(f);
				H_E[int(E1t)+2*m*n]+=1;}
				// no spin configuration update here. Reverse S[][] to s[][]
				S[i][j]=nn_old;
				// update boundaries back in order to use functions e_nergy and d_E!!
				if((i==1)||(i==m)||(j==1)||(j==n)){// update the big matrix boundaries
				 // top and bottom boundaries update
				 for(int j=0;j<(n+2);j++){S[0][j]=S[m][j];S[m+1][j]=S[1][j];}
				 // left and right boundaries update
				 for(int i=0;i<(m+2);i++){S[i][0]=S[i][n];S[i][n+1]=S[i][1];}}				
				}
		}

		MPI_Gather(H_E,n_odes,MPI_DOUBLE,H_E_total,n_odes,MPI_DOUBLE,0,MPI_COMM_WORLD);
		MPI_Gather(log_gE,n_odes,MPI_DOUBLE,log_gE_total,n_odes,MPI_DOUBLE,0,MPI_COMM_WORLD);

		if(id==0){

		    for(int i=0;i<n_odes;i++){
			for(int j=1;j<ntasks;j++){
			    H_E[i]+=   H_E_total[i+j*n_odes];
			    log_gE[i]+=log_gE_total[i+j*n_odes];}}

				ofstream gfile("log_gE_HE.txt");
				for(int i=0;i<n_odes;i++){
					gfile<<EpN[i]/(m*n)<<"\t"<<log_gE[i]<<"\t"<<H_E[i]<<"\n";}
				gfile.close();break;
				cout<<c_ount<<"\n";


			int max_H=1;
			for(int i=0;i<n_odes;i++){
				if(H_E[i]>=max_H){max_H=H_E[i];}}			
			int min_H=max_H;			
			for(int i=0;i<n_odes;i++){
				if(H_E[i]!=0 && H_E[i]<=min_H){min_H=H_E[i];}}

			r_atio=double(min_H)/double(max_H);  // do I need two doubles?
			//cout<<c_ount<<"\t"<<H_E[0]+H_E[1]<<"\t"<<H_E[4*m*n]<<"\t"<<r_atio<<"\n";		
			//if ((H_E[0]+H_E[1]>=0)&&(H_E[4*m*n-1]+H_E[4*m*n]>=0)&&(c_ount>=10000)&&(r_atio>=0.90)){
			if (r_atio>=0.90){
				cout<<c_ount<<"\t"<<E1t/(m*n)<<"\t"<<min_H<<"\t"<<max_H<<"\t"<<r_atio<<"\t"<<f<<"\n";
				ofstream gfile("log_gE_HE.txt");
				for(int i=0;i<n_odes;i++){
					gfile<<EpN[i]/(m*n)<<"\t"<<log_gE[i]<<"\t"<<H_E[i]<<"\n";}
				gfile.close();
				for(int i=0;i<n_odes;i++){H_E[i]=0;}
				f=sqrt(f);
				c_ount=0;}
			//F[0]=f;
			}
		//MPI_Bcast(H_E, n_odes, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		//MPI_Bcast(log_gE, n_odes, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		//MPI_Bcast(F, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		//f=F[0];
break;


	}
	
	
	// dynamic arrays clean up
	//************************
	delete[] EpN;
	
	delete[] s_data;
	delete[] s;
	
	delete[] S_data;
	delete[] S;
	
	delete[] cost_heta;
	delete[] cos_theta;
	
	delete[] log_gE;
	delete[] H_E;
	delete[] H_E_total;
	delete[] log_gE_total;
	delete[] F;

	MPI_Finalize();		
	return 0;
}
