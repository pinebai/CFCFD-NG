#!/bin/bash
echo "Check that simulation has converged by comparing solution instances:"
e3post.py --job=mms --tindx=6 \
    --compare-job=mms --compare-tindx=20
e3post.py --job=mms --tindx=7 \
    --compare-job=mms --compare-tindx=20

echo "----------------------------------------------------------------------"
echo "Check simulation against analytical data:"
e3post.py --job=mms --tindx=20 \
    --ref-function=analytic_solution_wrapper.py \
    --per-block-norm-list="0,rho,L2;0,rho,L1" \
    --global-norm-list="rho,L2"

echo "----------------------------------------------------------------------"
echo "Generate VTK files for plotting:"
e3post.py --job=mms --tindx=20 --vtk-xml

