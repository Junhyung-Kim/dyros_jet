% Produced by CVXGEN, 2018-03-22 04:11:37 -0400.
% CVXGEN is Copyright (C) 2006-2017 Jacob Mattingley, jem@cvxgen.com.
% The code in this file is Copyright (C) 2006-2017 Jacob Mattingley.
% CVXGEN, or solvers produced by CVXGEN, cannot be used for commercial
% applications without prior written permission from Jacob Mattingley.

% Filename: cvxsolve.m.
% Description: Solution file, via cvx, for use with sample.m.
function [vars, status] = cvxsolve(params, settings)
Ai = params.Ai;
Q = params.Q;
bi = params.bi;
c = params.c;
cvx_begin
  % Caution: automatically generated by cvxgen. May be incorrect.
  variable x(6, 1);

  minimize(quad_form(x, Q) + c'*x);
  subject to
    Ai*x <= bi;
cvx_end
vars.x = x;
status.cvx_status = cvx_status;
% Provide a drop-in replacement for csolve.
status.optval = cvx_optval;
status.converged = strcmp(cvx_status, 'Solved');
