/*
 * CurveFitting.h - Library for performing curve fitting.
 * Created by Savio Dimatteo, September 18, 2014.
 * Released into the public domain.
 */

#include "CurveFitting.h"
#include "Arduino.h"

CurveFitting::CurveFitting()
{
    _a = 0;
    _b = 0;
    _c = 0;
    _curveFitted = false;
}

void CurveFitting::fitPoints(double points[][2], int n)
{
   // sort array of points
   sort2DArray(points, n);

   // sums
   double sum1 = 0;
   double sum2 = 0;
   double sum3 = 0;
   double sum4 = 0;
   double sum5 = 0;

   double x1 = points[0][0];
   double y1 = points[0][1];

   double previous_sk = 0;
   double sk;
   for (int k=0; k<n; k++) {
      double xk = points[k][0];
      double yk = points[k][1];

      sk = previous_sk;
      if (k > 0) {
        sk = previous_sk + 0.5 * (yk + points[k-1][1]) * (xk + points[k-1][0]);
      }

      sum1 += ((xk - x1) * (xk - x1));
      sum2 += (xk - x1) * sk;
      sum3 += sk * sk;
      sum4 += ((yk - y1) * (xk - x1));
      sum5 += ((yk - y1) * sk);

      previous_sk = sk;
   }


   double rA1;
   double rB1;
   processMatrix(
      sum1, sum2, sum2, sum3, // the first matrix
      sum4, sum5,             // the vector 
      &rA1, &rB1              // the results go here
   );

   // tetha_k
   double c = rB1;
   double th[n];
   double sum_th = 0;
   for (int k=0; k<n; k++) {
      th[k] = exp(points[k][0] * c);
      sum_th += th[k];
   }

   // more sums
   double sum_th_th = 0;
   double sum_yk = 0;
   double sum_yk_thk = 0;
   for (int k=0; k<n; k++) {
     sum_th_th  += th[k] * th[k];
     sum_yk     += points[k][1];
     sum_yk_thk += points[k][1] * th[k];
   }

   double a;
   double b;
   processMatrix(
     n, sum_th, sum_th, sum_th_th,
     sum_yk, sum_yk_thk,
     &a, &b
   );

   _a = a;
   _b = b;
   _c = c;

   _curveFitted = true;
}

bool CurveFitting::isCurveFitted()
{
    return _curveFitted;
}

void CurveFitting::setParams(double a, double b, double c) {
   _a = a;
   _b = b;
   _c = c;

   _curveFitted = true;
}

double CurveFitting::estimate(double x)
{
    if (_curveFitted) { 
        return _a + ( _b * exp(_c * x) );
    }

    return NULL;
}


void CurveFitting::sort2DArray(double array[][2], int n)
{
    int max = n;
    while (max > 0) {
        
        for (int i = 0; i < max-1; i++) {
            double xa = array[i][0];
            double xb = array[i+1][0];

            if (xa > xb) {
                double ya = array[i][1];
                double yb = array[i+1][1];

                // swap x
                array[i][0] = xb;
                array[i+1][0] = xa;

                // swap y as well...
                array[i][1] = yb;
                array[i+1][1] = ya;
            }
        }

        max--;
    }
}

void CurveFitting::processMatrix(double a11, double a12, double a21, double a22, double b1, double b2, double *r1, double *r2) {
    double det = (a11 * a22) - (a21 * a12);
    double a11_inv = a22/det;
    double a12_inv = (-1 * a12)/det;
    double a21_inv = (-1 * a21)/det;
    double a22_inv = a11/det;

    *r1 = a11_inv * b1 + a12_inv * b2;
    *r2 = a21_inv * b1 + a22_inv * b2;
}

double CurveFitting::getEstimatedParameter(int parameter) {
    if (parameter == 0) {
        return _a;
    }
    else if (parameter == 1) {
        return _b;
    }
    else if (parameter == 2) {
        return _c;
    }
    return 0;
}
