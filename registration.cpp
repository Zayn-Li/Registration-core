#include <math.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

#include "mesh.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;

using namespace std;

// this function will pre-calculate two variables, SDF field and closest points pair, which are used in each iteration of registration
int RegistrationInit(const char* init_path, const char* track_path)    
{
    Mesh surf_init;
    Mesh surf_track;

    surf_init.ImportMeshFromPly(init_path); 
    surf_track.ImportMeshFromPly(track_path);

    static double SDF[100][100][100];
    static int id_close[100][100][100];
    double dist_new;
    double dist = 1;
    int index_new;

    // construct initial SDF
    double x_center,y_center,z_center,x_sum, y_sum, z_sum = 0;
    for (int i=0; i<surf_init.numVertices; i++)
    {
        x_sum = x_sum + surf_init.m_positions[i].x;
        y_sum = y_sum + surf_init.m_positions[i].y;
        z_sum = z_sum + surf_init.m_positions[i].z;
    }
    
	x_center = x_sum / surf_init.numVertices;  
    y_center = y_sum / surf_init.numVertices;   
    z_center = z_sum / surf_init.numVertices;      
    
    // use offset to translate the center of surface to origin

    double offset_x = x_center -0.05; //+0.05
    double offset_y = y_center -0.05; //+0.05
    double offset_z = z_center -0.05; //+0.05
    // the SDF grid of initial frame
    for (int i =0; i<100 ;i++)
    {
        for (int j =0; j<100 ;j++)
        {
            for (int k =0; k<100 ;k++)
            {
                dist = 1;
                // find the closest point to each grid vertex among all points in the initial surface. set the distance as SDF value of this grid vertex. 
                for (int i_surf_pt = 0; i_surf_pt<surf_init.numVertices; i_surf_pt++)
                {   
                    dist_new = pow(surf_init.m_positions[i_surf_pt].x - offset_x - i*0.001,2) + pow(surf_init.m_positions[i_surf_pt].y - offset_y - j*0.001,2)
                    + pow(surf_init.m_positions[i_surf_pt].z - offset_z - k*0.001,2);
                    if (dist_new < dist)
                    {
                        dist = dist_new;
                        index_new = i_surf_pt;
                    }
                }
                SDF[i][j][k] = dist;
                id_close[i][j][k] = index_new;
            }        
        }       
    }
    //point matching(tracking)
    // this part is contained in early versions and removed for simplicity.
    // make sure your inputs of the second function 'RegistrationError()' have the same number of pts and are all perfectly matched.

	
    // save the two pre-computed value under the same folder.
    // you may want to store this two values in memory in order to improve efficiency
    std::ofstream fp("./SDF.txt", std::ios::trunc);
 
	if (!fp.is_open())
	{
		printf("can't save SDF file\n");
		return 1;
	}
	for (int i = 0; i < 100; i++)
	{
        for (int j = 0; j < 100; j++)
	    {
        	for (int k = 0; k < 100; k++)
	        {
                fp << SDF[i][j][k];
                fp << " ";
            }
        }
	}
	fp.close();

	std::ofstream fp2("./closest_points.txt", std::ios::trunc);
 
	if (!fp2.is_open())
	{
		printf("can't save closest points\n");
		return 1;
	}
	for (int i = 0; i < 100; i++)
	{
        for (int j = 0; j < 100; j++)
	    {
        	for (int k = 0; k < 100; k++)
	        {
                fp2 << id_close[i][j][k];
                fp2 << " ";
            }
        }
	}
	fp2.close();
    return 0;
}

 // this function will compute the registration error and the correction term of each pt.
 // 'sim_path' and 'obs_path' are the two surface under comparation.
 // 'out_path2' saves the total registration error, 'out_path' saves the correction term (gradient of error) of each pt
 // IMPORTANT: the correction term shows the gradient of registration error. you should minus (but not add) this term to achieve better result.
int RegistrationError(const char* init_path, const char* sim_path, const char* obs_path, const char* out_path, const char* out_path2)   
{
    Mesh surf_init;
    Mesh surf_sim;
    Mesh surf_obs;

    surf_init.ImportMeshFromPly(init_path);
    surf_sim.ImportMeshFromPly(sim_path);
    surf_obs.ImportMeshFromPly(obs_path);

    // string file[17] = {"00","05","11","17","23","29","35","41","47","53","61","65","71","76","81","84","88"};
    double err_total;
    err_total = 0;
    double x_center,y_center,z_center,x_sum, y_sum, z_sum;
    x_center = 0;
    y_center = 0;
    z_center = 0;
    x_sum = 0;
    y_sum = 0;
    z_sum = 0;
    for (int i=0; i<surf_init.numVertices; i++)
    {
        x_sum = x_sum + surf_init.m_positions[i].x;
        y_sum = y_sum + surf_init.m_positions[i].y;
        z_sum = z_sum + surf_init.m_positions[i].z;
    }
    
	x_center = x_sum / surf_init.numVertices;  
    y_center = y_sum / surf_init.numVertices;   
    z_center = z_sum / surf_init.numVertices;      
    
    double offset_x = x_center -0.05; //+0.05
    double offset_y = y_center -0.05; //+0.05
    double offset_z = z_center -0.05; //+0.05

    // for(int i_file = 0;i_file<17;i_file++)
    // {
    // int i_file = 0;
    static double SDF[100][100][100];
    static int id_close[100][100][100];

    // static pcl::PointXYZ eul_deform[90][50][45];
    // Point3 eul_after_def[100][100][100];
    static double eul_after_def_x[100][100][100];
    static double eul_after_def_y[100][100][100];
    static double eul_after_def_z[100][100][100];
	
    
    // load pre-compute values.
    // this step can be time-consuming. try to save the values in memory if hardware permits.
    std::fstream fp("./SDF.txt", std::ios::in);
	if (!fp.is_open())
	{
		printf("can't find SDF file\n");
		return 1;
	}
	for (int i = 0; i < 100; i++)
	{
        for (int j = 0; j < 100; j++)
	    {
        	for (int k = 0; k < 100; k++)
	        {
                fp >> SDF[i][j][k];
            }
        }
	}
	fp.close();
	std::fstream fp2("./closest_points.txt", std::ios::in); 
	if (!fp2.is_open())
	{
		printf("can't find closest points\n");
		return 1;
	}
	for (int i = 0; i < 100; i++)
	{
        for (int j = 0; j < 100; j++)
	    {
        	for (int k = 0; k < 100; k++)
	        {
                fp2 >> id_close[i][j][k];
            }
        }
	}
	fp2.close();
    // deformation of surface point

    //printf("Init: %d Sim: %d\n",surf_init.numVertices,surf_sim.numVertices);
    Point3 pt_deform[surf_init.numVertices];
    for (int i_pt=0;i_pt<surf_init.numVertices;i_pt++)
    {
        pt_deform[i_pt].x = surf_obs.m_positions[i_pt].x-surf_init.m_positions[i_pt].x;
        pt_deform[i_pt].y = surf_obs.m_positions[i_pt].y-surf_init.m_positions[i_pt].y;
        pt_deform[i_pt].z = surf_obs.m_positions[i_pt].z-surf_init.m_positions[i_pt].z;
    }

    // the eular grid of simulated deformation
    
    for (int i =0; i<100 ;i++)
    {
        for (int j =0; j<100 ;j++)
        {
            for (int k =0; k<100 ;k++)
            {
                eul_after_def_x[i][j][k] = pt_deform[id_close[i][j][k]].x;
		        eul_after_def_y[i][j][k] = pt_deform[id_close[i][j][k]].y;
		        eul_after_def_z[i][j][k] = pt_deform[id_close[i][j][k]].z;
            }
        }
    }
   
    // find the interpolation coefficient

    double alpha,beta,gamma,alpha_g,beta_g,gamma_g,coeff_x,coeff_y,coeff_z;
    int i_grid,j_grid,k_grid,i_grid_g,j_grid_g,k_grid_g; 
    Point3 pt[8];  
    double err_grid[8];
    double err_pt[surf_obs.numVertices];
    // change the size for different dataset
    static double err_pt_grid[1000][11];

    for (int i_pt=0;i_pt<surf_obs.numVertices;i_pt++)
    {
        // find the interpolation coefficient
        i_grid = floor((surf_sim.m_positions[i_pt].x - offset_x)*1000);
        alpha = (surf_sim.m_positions[i_pt].x - offset_x)*1000 - i_grid;
        j_grid = floor((surf_sim.m_positions[i_pt].y - offset_y)*1000);
        beta = (surf_sim.m_positions[i_pt].y - offset_y)*1000 - j_grid;
        k_grid = floor((surf_sim.m_positions[i_pt].z - offset_z)*1000);
        gamma = (surf_sim.m_positions[i_pt].z - offset_z)*1000 - k_grid;             

        // inverse-transformation  
        for(int add_x=0; add_x<2;add_x++)
        {
            for(int add_y=0; add_y<2;add_y++)
            {
                for(int add_z=0; add_z<2;add_z++)
                {
                    pt[add_x*4+add_y*2+add_z].x = (i_grid+add_x) *0.001 - eul_after_def_x[i_grid+add_x][j_grid+add_y][k_grid+add_z];
                    pt[add_x*4+add_y*2+add_z].y = (j_grid+add_y) *0.001 - eul_after_def_y[i_grid+add_x][j_grid+add_y][k_grid+add_z];
                    pt[add_x*4+add_y*2+add_z].z = (k_grid+add_z) *0.001 - eul_after_def_z[i_grid+add_x][j_grid+add_y][k_grid+add_z];
                }            
            }            
        }
        // compute error
        for(int i_grid_pt=0; i_grid_pt<8;i_grid_pt++)
        {        
            i_grid_g = floor(pt[i_grid_pt].x*1000);
            alpha_g = pt[i_grid_pt].x*1000 - i_grid_g;
            j_grid_g = floor(pt[i_grid_pt].y*1000);
            beta_g = pt[i_grid_pt].y*1000 - j_grid_g;
            k_grid_g = floor(pt[i_grid_pt].z*1000);
            gamma_g = pt[i_grid_pt].z*1000 - k_grid_g;  
            err_grid[i_grid_pt] = 0;
            err_pt_grid[i_pt][i_grid_pt] = 0;
            
            for(int add_x=0; add_x<2;add_x++)
            {
                if(add_x==0) coeff_x=1-alpha_g;
                else coeff_x=alpha_g;

                for(int add_y=0; add_y<2;add_y++)
                {
                    if(add_y==0) coeff_y=1-beta_g;
                    else coeff_y=beta_g;                    
                    
                    for(int add_z=0; add_z<2;add_z++)
                    {
                        if(add_z==0) coeff_z=1-gamma_g;
                        else coeff_z=gamma_g;                         
                        // error of each grid vertice
                        err_grid[i_grid_pt] += coeff_x * coeff_y * coeff_z * SDF[i_grid_g+add_x][j_grid_g+add_y][k_grid_g+add_z];
                        
                    }            
                }            
            }
            err_pt_grid[i_pt][i_grid_pt] = err_grid[i_grid_pt];   

        }
        err_pt_grid[i_pt][8] = alpha;
        err_pt_grid[i_pt][9] = beta;
        err_pt_grid[i_pt][10] = gamma;
        // error of each point
        err_pt[i_pt] = alpha * beta * gamma * err_grid[7] + alpha * beta * (1-gamma) * err_grid[6] + alpha * (1-beta) * gamma * err_grid[5] + alpha * (1-beta) * (1-gamma) * err_grid[4]
                            + (1-alpha) * beta * gamma * err_grid[3] + (1-alpha) * beta * (1-gamma) * err_grid[2] + (1-alpha) * (1-beta) * gamma * err_grid[1] + (1-alpha) * (1-beta) * (1-gamma) * err_grid[0];
        err_total += err_pt[i_pt];
    }
    double err_temp;
    Mesh pt_dev;

    // find the derivation at each pt in three direction (x,y,z)
    pt_dev.ImportMeshFromPly(init_path);
    for(int i_dev_pt=0; i_dev_pt<surf_obs.numVertices; i_dev_pt++)
    {   
        alpha = err_pt_grid[i_dev_pt][8];
        beta = err_pt_grid[i_dev_pt][9];
        gamma = err_pt_grid[i_dev_pt][10];
        // 'dev_ax' indicates the direction of derivation
        for(int dev_ax=0; dev_ax<3; dev_ax++)
        {
            err_temp = 0;
            if(dev_ax==0) 
            {
                err_temp = 0.1 * beta * gamma * err_pt_grid[i_dev_pt][7] + 0.1 * beta * (1-gamma) * err_pt_grid[i_dev_pt][6] + 0.1 * (1-beta) * gamma * err_pt_grid[i_dev_pt][5] + 0.1 * (1-beta) * (1-gamma) * err_pt_grid[i_dev_pt][4]
                            - 0.1 * beta * gamma * err_pt_grid[i_dev_pt][3] - 0.1 * beta * (1-gamma) * err_pt_grid[i_dev_pt][2] - 0.1 * (1-beta) * gamma * err_pt_grid[i_dev_pt][1] - 0.1 * (1-beta) * (1-gamma) * err_pt_grid[i_dev_pt][0];                
                pt_dev.m_positions[i_dev_pt].x = err_temp * 10000;    
            
            }
            // error of each point
            if(dev_ax==1) 
            {
                err_temp = alpha * 0.1 * gamma * err_pt_grid[i_dev_pt][7] + alpha * 0.1 * (1-gamma) * err_pt_grid[i_dev_pt][6] + alpha * (-0.1) * gamma * err_pt_grid[i_dev_pt][5] + alpha * (-0.1) * (1-gamma) * err_pt_grid[i_dev_pt][4]
                            + (1-alpha) * 0.1 * gamma * err_pt_grid[i_dev_pt][3] + (1-alpha) * 0.1 * (1-gamma) * err_pt_grid[i_dev_pt][2] + (1-alpha) * (-0.1) * gamma * err_pt_grid[i_dev_pt][1] + (1-alpha) * (-0.1) * (1-gamma) * err_pt_grid[i_dev_pt][0];                
                pt_dev.m_positions[i_dev_pt].y = err_temp * 10000;    
            
            }           
            if(dev_ax==2) 
            {
                err_temp = alpha * beta * 0.1 * err_pt_grid[i_dev_pt][7] + alpha * beta * (-0.1) * err_pt_grid[i_dev_pt][6] + alpha * (1-beta) * 0.1 * err_pt_grid[i_dev_pt][5] + alpha * (1-beta) * (-0.1) * err_pt_grid[i_dev_pt][4]
                            + (1-alpha) * beta * 0.1 * err_pt_grid[i_dev_pt][3] + (1-alpha) * beta * (-0.1) * err_pt_grid[i_dev_pt][2] + (1-alpha) * (1-beta) * 0.1 * err_pt_grid[i_dev_pt][1] + (1-alpha) * (1-beta) * (-0.1) * err_pt_grid[i_dev_pt][0];               
                pt_dev.m_positions[i_dev_pt].z = err_temp * 10000;    
            
            }
        }
    }

    // save the derivation of error (correction term)
    // data is save in '.ply' format, and the headler makes no sense. 
    pt_dev.ExportToPly(out_path);
    
    // save the total error
	std::ofstream wfile(out_path2);

    if (wfile)
    {    
        wfile << "ERROR" << std::endl;
        wfile << err_total << std::endl;
    }

	// for (int i = 0; i < 100; i++)
	// {
    //     for (int j = 0; j < 60; j++)
	//     {
    //     	for (int k = 0; k < 55; k++)
	//         {
    //             SDF[i][j][k] = 0;
    //             id_close[i][j][k]
    //         }
    //     }
	// }    

    return 0;
}

// the interface for python to imply this two functions. 
PYBIND11_MODULE(registration, m) {
    m.doc() = "run .reg_init() first and then run .reg_err()";
    m.def("reg_init", &RegistrationInit, "Set up the SDF and closest points map");
    m.def("reg_err", &RegistrationError, "Calculate the registration error and gradient");
}
