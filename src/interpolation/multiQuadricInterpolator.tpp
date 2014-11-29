#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <Eigen/SparseQR>
#include <Eigen/Core>
#include <vector>

#include "coords.hpp"

using namespace std;
using namespace Eigen;

template <typename T, typename F>
MultiQuadricInterpolator<T,F>::MultiQuadricInterpolator(double delta_min, double delta_max) :
    Interpolator<T,F>(),
    delta_min(delta_min),
    delta_max(delta_max)
{
}

template <typename T, typename F>
MultiQuadricInterpolator<T,F>::~MultiQuadricInterpolator() {
}


template <typename T, typename F>
InterpolatedData<F> MultiQuadricInterpolator<T,F>::operator()(unsigned int Nx, unsigned int Ny,
 unsigned int nData, double *x, double *y, T* data) const{

    // valeurs finales évaluées 
    F* density = new F[Nx*Ny];
    F max = std::numeric_limits<F>::min();
    F min = std::numeric_limits<F>::max();

    // convertir les coordonées [longitude, latitude] => [0,1]^2
    Coords<double> unitCoords = toUnitSquare(Coords<double>(nData, x, y));


    // remplir le tableau de densité en interpolant les données
    // some sample data do not exists, we first need to check how many
    unsigned int n = nData;
    for (int i = 0; i < nData; ++i)
    {
        if (data[i] < T(0))
        {
            n--;
        }
    }
    SparseMatrix<double> M;
    M.resize(n,n);
    std::vector<Triplet<double>> trips(n);


    VectorXd b;
    b.resize(n);

    // filling matrix
    unsigned int i_safe = 0;
    unsigned int j_safe = 0;
    for (unsigned int i = 0; i < nData; i++){
        if (data[i] >= T(0)){
            b(i_safe) = data[i];
            i_safe ++;
        }
    }
    i_safe = 0;
    j_safe = 0;
    for (unsigned int j = 0; j < nData; j++){
        if (data[j] >= T(0)){
            i_safe = 0; 
            for (unsigned int i = 0; i < nData; i++){
                if (data[i] >= T(0)){
                    F w = hardyQuadric(i,unitCoords.x[j],unitCoords.y[j],unitCoords,n);
                    trips.push_back(Triplet<double>(i_safe, j_safe, w));
                    i_safe ++;
                }
            }
            j_safe ++;
        }
    }
    M.setFromTriplets(trips.begin(), trips.end());
    SparseQR<SparseMatrix<double>, COLAMDOrdering<int>> solverM;
    solverM.compute(M);


    // solve sysem
    VectorXd c = solverM.solve(b);


    unsigned int k_safe = 0;
    // remplir le tableau de densité en interpolant les données
    for (unsigned int j = 0; j < Ny; j++) {
        F d_y = static_cast<F>(j)/Ny;
        for (unsigned int i = 0; i < Nx; i++) {
            F d_x = static_cast<F>(i)/Nx;
            F d = F(0);
            k_safe = 0;
            for (unsigned int k = 0; k < nData; k++) {
                if(data[k] >= T(0)) { //if sampled data exists
                    F w = hardyQuadric(k,d_x,d_y,unitCoords,n);
                    d += w*F(c(k_safe));
                    k_safe ++;
                }
            }
            max = (d > max) ? d : max;
            min = (d < min) ? d : min;
            density[j*Nx+i] = d;
        }
    }

    return InterpolatedData<F>(density, min, max, Nx, Ny);
}
template <typename T, typename F>
F MultiQuadricInterpolator<T,F>::hardyQuadric(unsigned int k, F d_x, F d_y, Coords<double> unitCoords, unsigned int n) const{

    double delta_i = this->delta_min * std::pow((this->delta_max/this->delta_min),static_cast<F>(k-1)/(n-1));
    F x_k = unitCoords.x[k];
    F y_k = unitCoords.y[k];

    F n_x = norm(d_x,x_k);
    F n_y = norm(d_y,y_k);

    return sqrt(n_x*n_x + n_y*n_y + delta_i);


}

template <typename T, typename F>
F MultiQuadricInterpolator<T,F>::norm(F x1, F x2) const{
    return sqrt((x2-x1)*(x2-x1));
}
