/*
 * CurveFitting.h - Library for performing curve fitting.
 * Created by Savio Dimatteo, September 18, 2014.
 * Released into the public domain.
 */

#ifndef CurveFitting_h
#define CurveFitting_h

#include "Arduino.h"

class CurveFitting
{
    public:
        CurveFitting();
        void fitPoints(double points[][2], int n);
        double estimate(double x);
        bool isCurveFitted();
        double getEstimatedParameter(int parameter);
        void setParams(double a, double b, double c);

    private:
        void processMatrix(double a11, double a12, double a21, double a22, double b1, double b2, double *r1, double *r2);
        void sort2DArray(double array[][2], int n);

        // exponential parameters stored internally
        double _curveFitted;
        double _a;
        double _b;
        double _c;
};

#endif
