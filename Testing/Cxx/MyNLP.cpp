// Copyright (C) 2004, 2009 International Business Machines and others.
// All Rights Reserved.
// This code is published under the Eclipse Public License.
//
// Authors:  Carl Laird, Andreas Waechter     IBM    2004-11-05

#include <cassert>
#include <iostream>

#include "MyNLP.hpp"

using namespace Ipopt;

// constructor
MyNLP::MyNLP()
{}

// destructor
MyNLP::~MyNLP()
{}

// returns the size of the problem
bool MyNLP::get_nlp_info(
   Index&          n,
   Index&          m,
   Index&          nnz_jac_g,
   Index&          nnz_h_lag,
   IndexStyleEnum& index_style
)
{
   // The problem described in MyNLP.hpp has 2 variables, x1, & x2,
   n = 2;

   // one equality constraint,
   m = 1;

   // 2 nonzeros in the jacobian (derivative of g w.r.t. x)
   nnz_jac_g = 2;

   // and 2 nonzeros in the hessian of the lagrangian
   // (derivative of sigma*f + lambda*g w.r.t. x)
   nnz_h_lag = 2;

   // We use the standard fortran index style for row/col entries
   index_style = TNLP::FORTRAN_STYLE;

   return true;
}

// returns the variable bounds
bool MyNLP::get_bounds_info(
   Index   n,
   Number* x_l,
   Number* x_u,
   Index   m,
   Number* g_l,
   Number* g_u
)
{
   // here, the n and m we gave IPOPT in get_nlp_info are passed back to us.
   // If desired, we could assert to make sure they are what we think they are.
   assert(n == 2);
   assert(m == 1);

   // x1 has bounds [-1, 1]
   x_l[0] = -1.0;
   x_u[0] = 1.0;

   // x2 has no bounds
   x_l[1] = -1.0e19;
   x_u[1] = +1.0e19;

   // the constraint 0 = x1^2 + x2 - 1
   g_l[0] = g_u[0] = 0.0;

   return true;
}

// returns the initial point for the problem
bool MyNLP::get_starting_point(
   Index   n,
   bool    init_x,
   Number* x,
   bool    init_z,
   Number* z_L,
   Number* z_U,
   Index   m,
   bool    init_lambda,
   Number* lambda
)
{
   // Here, we assume we only have starting values for x, if you code
   // your own NLP, you can provide starting values for the others if
   // you wish.
   assert(init_x == true);
   assert(init_z == false);
   assert(init_lambda == false);

   // we initialize x in bounds, in the upper right quadrant
   x[0] = 0.5;
   x[1] = 1.5;

   return true;
}

// returns the value of the objective function
bool MyNLP::eval_f(
   Index         n,
   const Number* x,
   bool          new_x,
   Number&       obj_value
)
{
   assert(n == 2);

   obj_value = -(x[1] - 2.0) * (x[1] - 2.0);

   return true;
}

// return the gradient of the objective function grad_{x} f(x)
bool MyNLP::eval_grad_f(
   Index         n,
   const Number* x,
   bool          new_x,
   Number*       grad_f
)
{
   assert(n == 2);

   grad_f[0] = 0.0;
   grad_f[1] = -2.0 * (x[1] - 2.0);

   return true;
}

// return the value of the constraints: g(x)
bool MyNLP::eval_g(
   Index         n,
   const Number* x,
   bool          new_x,
   Index         m,
   Number*       g
)
{
   assert(n == 2);
   assert(m == 1);

   g[0] = x[0] * x[0] + x[1] - 1.0;

   return true;
}

// return the structure or values of the jacobian
bool MyNLP::eval_jac_g(
   Index         n,
   const Number* x,
   bool          new_x,
   Index         m,
   Index         nele_jac,
   Index*        iRow,
   Index*        jCol,
   Number*       values
)
{
   assert(n == 2);
   assert(m == 1);
   assert(nele_jac == 2);

   if( values == NULL )
   {
      // return the structure of the jacobian of the constraints

      // constraint 0 depends on x0 and x1
      iRow[0] = 1;
      jCol[0] = 1;
      iRow[1] = 1;
      jCol[1] = 2;
   }
   else
   {
      // return the values of the jacobian of the constraints
      values[0] = 2.0 * x[0]; // derivative of g w.r.t. x1
      values[1] = 1.0;        // derivative of g w.r.t. x2
   }

   return true;
}

// return the structure or values of the hessian
bool MyNLP::eval_h(
   Index         n,
   const Number* x,
   bool          new_x,
   Number        obj_factor,
   Index         m,
   const Number* lambda,
   bool          new_lambda,
   Index         nele_hess,
   Index*        iRow,
   Index*        jCol,
   Number*       values
)
{
   assert(n == 2);
   assert(m == 1);
   assert(nele_hess == 2);

   if( values == NULL )
   {
      // return the structure. This is a symmetric matrix, fill the lower left
      // triangle only.

      // the hessian for this problem is actually dense
      iRow[0] = 1;
      jCol[0] = 1;
      iRow[1] = 2;
      jCol[1] = 2;
   }
   else
   {
      // return the values. This is a symmetric matrix, fill the lower left
      // triangle only

      // fill the objective portion
      values[0] = 0.0;                // 2nd derivative of f w.r.t. x1
      values[1] = obj_factor * (-2.0); // 2nd derivative of f w.r.t. x2

      // add the portion for the constraint
      values[0] += lambda[0] * 2.0; // 2nd derivative of g w.r.t. x1
      values[1] += 0.0;             // 2nd derivative of g w.r.t. x2
   }

   return true;
}

void MyNLP::finalize_solution(
   SolverReturn               status,
   Index                      n,
   const Number*              x,
   const Number*              z_L,
   const Number*              z_U,
   Index                      m,
   const Number*              g,
   const Number*              lambda,
   Number                     obj_value,
   const IpoptData*           ip_data,
   IpoptCalculatedQuantities* ip_cq
)
{
   // here is where we would store the solution to variables, or write to a file, etc
   // so we could use the solution. Since the solution is displayed to the console,
   // we currently do nothing here.
}
