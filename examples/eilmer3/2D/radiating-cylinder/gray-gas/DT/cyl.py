## \file infinite-slab.py
## \brief Test case for radiation-transport models
## \author DFP, 06-Dec-2008

job_title = "Constant property slab at S. Karl test-case conditions."
print job_title

# job control parameters
LINEAR_GRADIENT = False
L_SCALE = 1.0
NXBLOCKS = 2
NYBLOCKS = 1
nnx = 16        
nny = 16      
ar = 0.1       # aspect ratio

# We can set individual attributes of the global data object.
gdata.title = job_title
gdata.axisymmetric_flag = 1 
gdata.viscous_flag = 1
gdata.dt = 1.0e-9
gdata.max_time = 1.0e-4
gdata.fixed_time_step = False
gdata.dt_plot = 2.0e-05
gdata.max_step = 1

select_gas_model(model='ideal gas', species=['air'])
global gm
gm = get_gas_model_ptr()
global nsp, ntm
nsp = gm.get_number_of_species()
ntm = gm.get_number_of_modes()
select_radiation_model( input_file="gray-gas-DT.lua", update_frequency=1 )

# Define flow conditions
global u, v, w
u = 0.0; v = 0.0; w = 0.0
global p
p = 1.0e4
global T_i, T_f
T_i = 0.0; T_f = 1.0e4
global f
f = [ 1.0 ]

def linear_T_gradient(x,y,z):
    # define a linear T gradient over 1m in the y direction
    # simple simple
    T = y * ( T_f - T_i )
    # setup newgas
    newgas = Gas_data(gm)
    newgas.p = p
    newgas.T[0] = T
    for itm in range(ntm):
        newgas.T[itm] = T
    for isp in range(nsp):
        newgas.massf[isp] = f[isp]
    gm.eval_thermo_state_pT(newgas)
    gm.eval_thermo_state_rhoe(newgas)
    data = {'vel.x':u, 'vel.y':v, 'vel.z':w,
            'tke':0.0, 'omega':0.0, 'mu_t':0.0, 'k_t':0.0, 'S':0}
    data['rho'] = newgas.rho 
    data['p'] = newgas.p 
    data['a'] = newgas.a
    data['mu'] = newgas.mu
    data['sigma_T'] = 0.0
    data['sigma_c'] = 0.0
    for isp in range(nsp):
        sp = gm.species_name(isp)
        data['massf[%d]-%s' % ( isp, sp )] = newgas.massf[isp]
    for itm in range(ntm):
        data['e[%d]' % itm] = newgas.e[itm]
        data['T[%d]' % itm] = newgas.T[itm]
        data['k[%d]' % itm] = newgas.k[itm]
    
    return data

if LINEAR_GRADIENT:
    initial = linear_T_gradient
else:
    initial = FlowCondition(p=p,  u=0.0, v=0.0, T=T_f,  massf=f)

# Define the geometry
a = Node(           0.0,        0.00, label="A")
b = Node(1.0*L_SCALE/ar,        0.00, label="B")
c = Node(1.0*L_SCALE/ar, 1.0*L_SCALE, label="C")
d = Node(           0.0, 1.0*L_SCALE, label="D")

ab = Line(a, b) # southern boundary
bc = Line(b, c) # eastern boundary
dc = Line(d, c) # northern boundary
ad = Line(a, d) # western boundary

# Define the block and discretisation.
blk_0 = SuperBlock2D(make_patch(dc, bc, ab, ad),
                     nni=nnx, nnj=nny, nbi=NXBLOCKS, nbj=NYBLOCKS,
                     fill_condition=initial, label="BLOCK-0")

identify_block_connections()

sketch.xaxis(0.0, 2.0,  0.4,   -0.05)
sketch.yaxis(0.0, 0.02, 0.004, -0.0005)
sketch.window(0.0, 0.0, 2.0, 1.0, 0.01, 0.01, 0.17, 0.17)
    