#!/usr/bin/env python
"""
e3_load_balance.py -- Program to do some static load balancing.

The idea of this program routines is to re-order a supplied
hostfile in such a way as to give a decent load balancing when
running MPI in an oversubscribed manner. What we are trying to solve
is what is referred to as the "multiprocessor scheduling
problem", which may be stated as [paraphrasing Graham (1969)]:

Let us suppose we are given 'n' identical processing
units P_i, i = 1, ..., n, and a set of tasks,
T = {T_i, ...., T_r} which is to be processed by
the P_i. We also have a measure of the time it
takes to complete each task, mu(T_i).  The total
time taken to complete the tasks is 'w'.

In our restricted problem, we wish to find
the best way to assign the T_i to the P_i
which minimises 'w'. We have no priority
list for the task, that is, any task may 
start at any time the resources are free,
and is not conditional on a prior task
finishing.

We will do the task assignment in a static manner. The
estimates of computed load per block are simply based
on the block dimensions. These estimates may be poor
if we are doing a lot of work in certain cells compared
to others due to chemistry or thermal nonequilibrium,
for example.

The algorithm chosen [Graham (1969)] is not provably
optimal, but it should generally give a good result
for a relatively cheap computation. It is possible to
find the optimimum task packing by brute force enumerating
all possible ways the tasks could be packed into the
processing units, but the computational expense of
this increases exponentially with number of tasks
and number of processing units.

Reference:
---------
Graham, R. L. (1979)
Bounds on Multiprocessing Timing Anomalies,
SIAM Journal of Applied Mathematics, 17:2, pp. 416--429

.. Author: Rowan J. Gollan
.. Versions:
   21-Aug-2012: initial coding.
"""

import sys
import os
import ConfigParser
from copy import copy
from optparse import OptionParser

parser = OptionParser()
parser.add_option("-j", "--job", dest="jobname", type="string",
                  help="base file name of job",
                  metavar="JOBNAME")
parser.add_option("-i", "--input-hostfile", dest="hosts_in", type="string",
                  help="supplied hostfile (input)",
                  metavar="HOSTS_IN")
parser.add_option("-o", "--output-hostfile", dest="hosts_out", type="string",
                  help="output hostfile with entries re-ordered",
                  metavar="HOSTS_OUT")

def create_task_list(ctrl_fname):
    """
    Return a list of tuples of form (task_id, task_load).

    The returned list will have one entry per block.
    It will also be sorted based on load grom highest
    to lowest.
    
    :param ctrl_fname: Name of eilmer control file.
    """
    cp = ConfigParser.ConfigParser()
    cp.read(ctrl_fname)
    nblks = int(cp.get("control_data", "nblock"))

    tasks = []
    for ib in range(nblks):
        sec_name = "block/" + str(ib)
        nni = int(cp.get(sec_name, "nni"))
        nnj = int(cp.get(sec_name, "nnj"))
        nnk = int(cp.get(sec_name, "nnk"))
        # Estimate task load as: nni*nnj*nnk
        tasks.append((ib, nni*nnj*nnk))
    #
    # Sort tasks by load (2nd element in tuple)
    tasks.sort(key=lambda x: x[1], reverse=True)
    return tasks

def parse_hosts_file(fname):
    f = open(fname, 'r')
    hosts = []
    while True:
        line = f.readline()
        if not line:
            break
        hosts.append(line.split()[0])
    #
    return hosts

def main():
    (options, args) = parser.parse_args()
    
    if options.jobname is None:
        print "The base file name for the job must be specified with"
        print "--job=JOBNAME"
        print parser.print_help()
        sys.exit(1)
    if options.hosts_in is None:
        print "The input host file name must be specified with"
        print "--input-hostfile=HOSTS_IN"
        print parser.print_help()
        sys.exit(1)
    if options.hosts_out is None:
        print "The output host file name must be specified with"
        print "--output-hostfile=HOSTS_OUT"
        print parser.print_help()
        sys.exit(1)

    ctrl_fname = options.jobname + ".control"
    tasks = create_task_list(ctrl_fname)
    hosts = parse_hosts_file(options.hosts_in)
    nprocs = len(hosts)

    if nprocs > len(tasks):
        print "e3loadbalance.py: Quitting without doing anything"
        print "because nprocs > ntasks."
        print "nprocs= ", nprocs
        print "ntasks= ", ntasks
        print "Done."
        sys.exit(0)
    #
    procs = []
    for i in range(nprocs): procs.append([])
    proc_loads = [0,]*nprocs
    task_hosts = []
    for (task_id, task_load) in tasks:
        # The algorithm is simply this:
        # Given a list sorted from heaviest load to lightest,
        # at any stage, assign task to the processing unit
        # with the minimum load currently.
        min_load = min(proc_loads)
        imin = proc_loads.index(min_load)
        proc_loads[imin] += task_load
        procs[imin].append(task_id)
        task_hosts.append(imin)
    #
    # Before writing out re-ordered host file,
    # first print up the blocks per host to
    # screen.
    for ip, tsk_list in enumerate(procs):
        print "For processing unit: ", ip, " hostname: ", hosts[ip]
        print "The assigned blocks assigned are...."
        for t in tsk_list:
            print t,
        print ""
    #
    # Now write out new host list.
    print "Writing new host file to: ", options.hosts_out
    f = open(options.hosts_out, 'w')
    for proc_id in task_hosts:
        f.write("%s\n" % hosts[proc_id])
    f.close()
    print "Done."
    #
    print "e3loadbalance.py: Done."
    return

if __name__ == '__main__':
    main()


    





