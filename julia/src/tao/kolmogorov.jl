#
# kolmogorov.jl --
#
# Generation of a Kolmogorov phase screen.
#
#-------------------------------------------------------------------------------
#
# This file if part of the TAO software (https://github.com/emmt/TAO) licensed
# under the MIT license.
#
# Copyright (C) 2018, Éric Thiébaut.
#

using Random

"""

```julia
kolmogorov(diam, r0=diam; all=false, orig=false, seed=-1)
```

yields an array of random phases which follow Kolmogorov law on a square pupil
of `diam` samples per side with a Fried's parameter equal to `r0` (in units of
the sampling step, default is to set `r0 = diam`).  If a Point Spread Function
is to be calculated from the generated phase screen, it should be conveniently
sampled (i.e., `r0` greater or equal 2 or 3 samples).

The algorithm is the mid-point method of R.G. Lane, A. Glindemann and
J.C. Dainty (``Simulation of a Kolmogorov phase screen'' in Waves in Random
Media, 2, 209-224, 1992).  Boolean keyword `orig` indicates whether or not the
original method by Lane et al. should be used (default is to not use the
original algorithm).

Boolean keyword `all` indicates whether or not all the computed phase screen
should be returned.  The default behaviour is to return the smallest array into
which a pupil with a diameter of `diam` samples can fit.  The computed phase
screen is a `(2^n+1)×(2^n+1)` array with `n` integer.

Keyword `seed` can be set with a nonnegative integer to reinitialized the
random sequence (see [`Random.seed!`](@ref)).

"""
function kolmogorov(diam::Integer, r0::Real=diam;
                    all::Bool=false,
                    orig::Bool=false,
                    seed::Integer=-1)
    if seed ≥ 0
        # Seed the random generator and reset related storage.
        Random.seed!(seed)
    end

    # Compute the size of the minimum array which holds the phase screen over
    # the pupil.
    n = (1 << ceil(Int, log2(diam - 1))) + 1
    delta = sqrt(6.88*(diam/r0)^(5/3))

    beta = 1.7817974
    c1 = 3.3030483e-1*delta
    c2 = 6.2521894e-1*delta
    c3 = 5.3008502e-1*delta
    c4 = 3.9711507e-1*delta
    if orig
        c5 = 4.5420202e-1*delta
    else
        c5 = 4.4355177e-1*delta
        l5 = 4.5081546e-1
        m5 = 9.8369088e-2
    end
    a = randn((n, n))
    b1 = c2*randn()
    b2 = c2*randn()

    # 4 first corners
    a[1,1] = c1*a[1,1] + b1
    a[n,1] = c1*a[n,1] + b2
    a[1,n] = c1*a[1,n] - b2
    a[n,n] = c1*a[n,n] - b1

    # all other points
    h = n - 1
    @inbounds while h ≥ 2
        s = h   # step size
        h >>= 1 # half the step size
        c3 /= beta
        c4 /= beta
        c5 /= beta

        # centre of squares
        for k in 1+h:s:n-h        # mid-point coordinates
            kp, kn = k-h, k+h     # previous and next points
            for j in 1+h:s:n-h    # mid-point coordinates
                jp, jn = j-h, j+h # previous and next points
                a[j,k] = c3*a[j,k] + 0.25*(a[jp,kp] + a[jp,kn] +
                                           a[jn,kp] + a[jn,kn])
            end
        end

        if 2s < n
            # centre of losanges
            for k in 1+s:s:n-s        # vertice coordinates
                kp, kn = k-h, k+h     # previous and next points
                for j in 1+h:s:n-h    # mid-point coordinates
                    jp, jn = j-h, j+h # previous and next points
                    a[j,k] = c4*a[j,k] + 0.25*(a[j,kp] + a[j,kn] +
                                               a[jp,k] + a[jn,k])
                    a[k,j] = c4*a[k,j] + 0.25*(a[k,jp] + a[k,jn] +
                                               a[kp,j] + a[kn,j])
                end
            end
        end

        # borders
        if orig
            for j in 1+h:s:n-h    # mid-point coordinates
                jp, jn = j-h, j+h # previous and next points
                a[1,j] = c5*a[1,j] + 0.5*(a[1,jp] + a[1,jn])
                a[n,j] = c5*a[n,j] + 0.5*(a[n,jp] + a[n,jn])
                a[j,1] = c5*a[j,1] + 0.5*(a[jp,1] + a[jn,1])
                a[j,n] = c5*a[j,n] + 0.5*(a[jp,n] + a[jn,n])
            end
        else
            for j in 1+h:s:n-h    # mid-point coordinates
                jp, jn = j-h, j+h # previous and next points
                a[1,j] = c5*a[1,j] + l5*(a[1,jp] + a[1,jn]) + m5*a[h,j]
                a[n,j] = c5*a[n,j] + l5*(a[n,jp] + a[n,jn]) + m5*a[n-h,j]
                a[j,1] = c5*a[j,1] + l5*(a[jp,1] + a[jn,1]) + m5*a[j,h]
                a[j,n] = c5*a[j,n] + l5*(a[jp,n] + a[jn,n]) + m5*a[j,n-h]
            end
        end
    end

    if all
        return a
    else
        t = (n - diam) >> 1
        rng = 1+t:diam+t
        return a[rng, rng]
    end
end
