-- udf-source-template.lua
-- Lua template for the source terms of a Manufactured Solution.
--
-- PJ, 29-May-2011
-- RJG, 06-Jun-2014
--   Declared maths functions as local

-- dummy functions to keep eilmer3 happy
function at_timestep_start(args) return nil end
function at_timestep_end(args) return nil end

local sin = math.sin
local cos = math.cos
local exp = math.exp
local pi = math.pi

function source_vector(args, cell)
   src = {}
   x = cell.x
   y = cell.y
<insert-source-terms-here>
   src.mass = fmass
   src.momentum_x = fxmom
   src.momentum_y = fymom
   src.momentum_z = 0.0
   src.total_energy = fe
   src.species = {}
   src.species[0] = src.mass
   return src
end
