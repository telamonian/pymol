# Generic vector and matrix routines for 3-Space
# Assembled for usage in PyMOL and Chemical Python
#
# Assumes row-major matrices and arrays
# [ [vector 1], [vector 2] ]
#
# Raises ValueError when given bad input
#
# TODO: documentation!

import math
import whrandom

RSMALL4 = 0.0001

#------------------------------------------------------------------------------
def get_null():
   return [0.0,0.0,0.0]

#------------------------------------------------------------------------------   
def get_identity():
   return [[1.0,0.0,0.0],[0.0,1.0,0.0],[0.0,0.0,1.0]]

#------------------------------------------------------------------------------
def distance_sq(v1, v2):
   d0 = v2[0] - v1[0]
   d1 = v2[1] - v1[1]
   d2 = v2[2] - v1[2]
   return (d0*d0) + (d1*d1) + (d2*d2)

#------------------------------------------------------------------------------
def distance_sq(v1, v2):
   d0 = v2[0] - v1[0]
   d1 = v2[1] - v1[1]
   d2 = v2[2] - v1[2]
   return (d0*d0) + (d1*d1) + (d2*d2)

#------------------------------------------------------------------------------
def distance(v1, v2):
   d0 = v2[0] - v1[0]
   d1 = v2[1] - v1[1]
   d2 = v2[2] - v1[2]
   return math.sqrt((d0*d0) + (d1*d1) + (d2*d2))

#------------------------------------------------------------------------------
def random_sphere(v,dist):
   return add(v,scale(normalize([
      whrandom.random()-0.5,whrandom.random()-0.5,whrandom.random()-0.5]),dist))

#------------------------------------------------------------------------------
def add(v1,v2):
   return [v1[0]+v2[0],v1[1]+v2[1],v1[2]+v2[2]]

#------------------------------------------------------------------------------
def scale(v,factor):
   return [v[0]*factor,v[1]*factor,v[2]*factor]

#------------------------------------------------------------------------------
def negate(v):
   return [-v[0],-v[1],-v[2]]

#------------------------------------------------------------------------------
def sub(v1,v2):
   return [v1[0]-v2[0],v1[1]-v2[1],v1[2]-v2[2]]

#------------------------------------------------------------------------------
def dot_product(v1,v2):
  return [v1[0]*v2[0],v1[1]*v2[1],v1[2]*v2[2]]

#------------------------------------------------------------------------------
def cross_product(v1,v2):
  return [(v1[1]*v2[2]) - (v1[2]*v2[1]),
          (v1[2]*v2[0]) - (v1[0]*v2[2]),
          (v1[0]*v2[1]) - (v1[1]*v2[0])]

#------------------------------------------------------------------------------
def transform(m,v):
   return [m[0][0]*v[0] + m[0][1]*v[1] + m[0][2]*v[2],
           m[1][0]*v[0] + m[1][1]*v[1] + m[1][2]*v[2],
           m[2][0]*v[0] + m[2][1]*v[1] + m[2][2]*v[2]]

#------------------------------------------------------------------------------
def transform_about_point(m,v,p):
   return add(transform(m,sub(v,p)),p)

#------------------------------------------------------------------------------
def get_angle(v1,v2):
   denom = (math.sqrt(((v1[0]*v1[0]) + (v1[1]*v1[1]) + (v1[2]*v1[2]))) *
            math.sqrt(((v2[0]*v2[0]) + (v2[1]*v2[1]) + (v2[2]*v2[2]))))
   if denom>RSMALL4:
      result = ( (v1[0]*v2[0]) + (v1[1]*v2[1]) + (v1[2]*v2[2]) ) / denom
   else:
      result = 0.0
   result = math.acos(0.0)
   return result

#------------------------------------------------------------------------------
def normalize(v):
   vlen = math.sqrt((v[0]*v[0]) + (v[1]*v[1]) + (v[2]*v[2]))
   if vlen>RSMALL4:
      return [v[0]/vlen,v[1]/vlen,v[2]/vlen]
   else:
      return get_null()

#------------------------------------------------------------------------------
def normalize_failsafe(v):
   vlen = math.sqrt((v[0]*v[0]) + (v[1]*v[1]) + (v[2]*v[2]))
   if vlen>RSMALL4:
      return [v[0]/vlen,v[1]/vlen,v[2]/vlen]
   else:
      return [1.0,0.0,0.0]

#------------------------------------------------------------------------------
def rotation_matrix(angle,axis):
   
   x=axis[0]
   y=axis[1]
   z=axis[2]
   
   s = math.sin(angle)
   c = math.cos(angle)

   mag = math.sqrt( x*x + y*y + z*z )

   if abs(mag)<RSMALL4:
      return get_identity()
   
   x = x / mag
   y = y / mag
   z = z / mag
 
   xx = x * x
   yy = y * y
   zz = z * z
   xy = x * y
   yz = y * z
   zx = z * x
   xs = x * s
   ys = y * s
   zs = z * s
   one_c = 1.0 - c
 
   return [[ (one_c * xx) + c , (one_c * xy) - zs, (one_c * zx) + ys],
           [ (one_c * xy) + zs, (one_c * yy) + c , (one_c * yz) - xs],
           [ (one_c * zx) - ys, (one_c * yz) + xs, (one_c * zz) + c ]]

#------------------------------------------------------------------------------
def transform_array(rot_mtx,vec_array):

   '''transform_array( matrix, vector_array ) -> vector_array

   '''

   return map( lambda x,m=rot_mtx:transform(m,x), vec_array )

#------------------------------------------------------------------------------
def translate_array(trans_vec,vec_array):

   '''translate_array(trans_vec,vec_array) -> vec_array

   Adds 'mult'*'trans_vec' to each element in vec_array, and returns
   the translated vector.
   '''

   return map ( lambda x,m=trans_vec:add(m,x),vec_array )

#------------------------------------------------------------------------------
def fit_apply(fit_result,vec_array):
   '''fit_apply(fir_result,vec_array) -> vec_array
   
   Applies a fit result to an array of vectors
   '''

   return map( lambda x,t1=fit_result[0],mt2=negate(fit_result[1]),
      m=fit_result[2]: add(t1,transform(m,add(mt2,x))),vec_array)

#------------------------------------------------------------------------------
def fit(target_array, source_array):

   '''fit(target_array, source_array) -> (t1, t2, rot_mtx, rmsd) [fit_result]

   Calculates the translation vectors and rotation matrix required
   to superimpose source_array onto target_array.  Original arrays are
   not modified.  NOTE: Currently assumes 3-dimensional coordinates

   t1,t2 are vectors from origin to centers of mass...
   '''

# Check dimensions of input arrays
   if len(target_array) != len(source_array):
      print ("Error: arrays must be of same length for RMS fitting.")
      raise ValueError
   if len(target_array[0]) != 3 or len(source_array[0]) != 3:
      print ("Error: arrays must be dimension 3 for RMS fitting.")
      raise ValueError
   nvec = len(target_array)
   ndim = 3
   maxiter = 200
   tol = 0.001

# Calculate translation vectors (center-of-mass).

   t1 = get_null()
   t2 = get_null()
   tvec1 = get_null()
   tvec2 = get_null()

   for i in range(nvec):
      for j in range(ndim):
         t1[j] = t1[j] + target_array[i][j]
         t2[j] = t2[j] + source_array[i][j]
   for j in range(ndim):
      t1[j] = t1[j] / nvec
      t2[j] = t2[j] / nvec

# Calculate correlation matrix.

   corr_mtx = []
   for i in range(ndim):
      temp_vec = []
      for j in range(ndim):
         temp_vec.append(0.0)
      corr_mtx.append(temp_vec)

   rot_mtx = []
   for i in range(ndim):
      temp_vec = []
      for j in range(ndim):
         temp_vec.append(0.0)
      rot_mtx.append(temp_vec)
   for i in range(ndim):
      rot_mtx[i][i] = 1.

   for i in range(nvec):
      for j in range(ndim):
         tvec1[j] = target_array[i][j] - t1[j]
         tvec2[j] = source_array[i][j] - t2[j]
      for j in range(ndim):
         for k in range(ndim):
            corr_mtx[j][k] = corr_mtx[j][k] + tvec2[j]*tvec1[k]

# Main iteration scheme (hardwired for 3X3 matrix, but could be extended).

   iters = 0
   while (iters < maxiter):
      iters = iters + 1
      ix = (iters-1)%ndim
      iy = iters%ndim
      iz = (iters+1)%ndim
      sig = corr_mtx[iz][iy] - corr_mtx[iy][iz]
      gam = corr_mtx[iy][iy] + corr_mtx[iz][iz]

      sg = (sig**2 + gam**2)**0.5
      if sg != 0.0 and (abs(sig) > tol*abs(gam)):
         sg = 1.0 / sg
         for i in range(ndim):

            bb = gam*corr_mtx[iy][i] + sig*corr_mtx[iz][i]
            cc = gam*corr_mtx[iz][i] - sig*corr_mtx[iy][i]
            corr_mtx[iy][i] = bb*sg
            corr_mtx[iz][i] = cc*sg

            bb = gam*rot_mtx[iy][i] + sig*rot_mtx[iz][i]
            cc = gam*rot_mtx[iz][i] - sig*rot_mtx[iy][i]
            rot_mtx[iy][i] = bb*sg
            rot_mtx[iz][i] = cc*sg

      else:
# We have a converged rotation matrix.  Calculate RMS deviation.
         vt1 = translate_array(negate(t1),target_array)
         vt2 = translate_array(negate(t2),source_array)
         vt3 = transform_array(rot_mtx,vt2)
         rmsd = 0.0
         for i in range(nvec):
            rmsd = rmsd + distance_sq(vt1[i], vt3[i])
         rmsd = math.sqrt(rmsd/nvec)
         return(t1, t2, rot_mtx, rmsd)

# Too many iterations; something wrong.
   print ("Error: Too many iterations in RMS fit.")
   raise ValueError

