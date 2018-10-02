# Detector calibration

We consider means to calibrate an image detector (CCD or CMOS) so as to have
images with pixel values proportionanl to the illumination and with an
estimator of the variances (or statistical weights) of those pixel values.

The model of raw detector images, their expectation and their variance
(assuming linear response and no clipping, all operations elementwise) are
given by:

```
     raw = (tr⋅src + bg)⋅Δt/gain + bias + noise
  E(raw) = (tr⋅src + bg)⋅Δt/gain + bias
Var(raw) = (tr⋅src + bg)⋅Δt/gain² + ron²
```

with

- `tr` = effective transmission (dimensionless), accounts for the detector
  quantum efficiency (QE) and optical throughput;

- `src` = source of interest, gives the expected number of incident photons per
  pixel per frame which are due to the source of interest;

- `bg` = effective background contribution (in electrons per pixel per unit of
  time), accounts for spurious sky background, instrumental thermal emission
  and detector dark current;

- `Δt` = exposure time;

- `gain` = detector gain (in electrons per digital level);

- `bias` = detector bias (in digital levels);

- `noise` = combination of Poissonian noise, readout noise, and digitization
  noise (in digital levels per pixel per frame);

- `ron` = standard deviation of the readout plus digitization noise (in digital
  levels per pixel per frame).

Note that `tr⋅src` is the mean number of photo-electrons (per pixel per unit of
time) generated in the detector due to the source of interest.

Assuming the same exposure time `Δt` for all frames, then the model simplifies
to:

```
    raw = tr⋅fg/gain + b + noise
  E(raw) = tr⋅fg/gain + b
Var(raw) = tr⋅fg/gain² + σ²
```

where:

- `fg = src⋅Δt` is the mean number of photons per pixel per frame due to the
  source of interest;

- `b = bg⋅Δt/gain + bias` is the offset (in digital levels) due to the detector
  bias and spurious background sources;

- `σ = sqrt(bg⋅Δt/gain² + ron²)` is the standard deviation of the detector plus
  background sources noise (in digital levels per pixel per frame).

The quantity of interest is `fg` and the unknown (pixelwise) parameters needed
to estimate `fg` are: `tr`, `gain`, `b` and `σ`.

A first sequence of *dark* images are obtained with `fg = 0`, their (empirical)
pixelwise mean and variance yield:

```
md ≡   E(dark) = b                                           (1)
vd ≡ Var(dark) = σ²                                          (2)
```

which provide 2 of the 4 unknown calibration parameters.

Note that these images should be more exactly called *background* images.

A second sequence of images of a *stable bright* source is needed.  That is,
`fg > 0` everywhere and does not depend on time.  Providing, the (empirical)
pixelwise mean and variance of the images of the *stable* source yield:

```
ms ≡   E(bright) = lvl + b                                   (3)
vs ≡ Var(bright) = lvl/gain + σ²                             (4)
```

where `lvl = tr⋅fg/gain` is the (pixelwise) digital level due to the light
source.  This level may be unknown but, again because this is important, it
has to be stable during the acquisition of the *bright* images.

Combining equations (1)-(4) to eliminate `lvl` yields:

```
gain = [E(bright) - E(dark)]/[Var(bright) - Var(dark)]
     = (ms - md)/(vs - vd)                                   (5)
```

incidentally, `lvl = E(bright) - E(dark) = ms - md` .

Equations (1), (2) and (5) gives 3 out of 4 calibration parameters.

To estimate the missing parameter `tr`, a third sequence of *flat* images is
needed.  These images are obtained, with a uniform (on average) illumination at
the entrance of the instrument.  The illumination needs not be stable in time
(only its pixelwise empirical mean is needed) but must be spatially uniform (or
at least very smooth) on average.  The pixelwise empirical mean of these *flat*
images yields:

```
mf ≡ E(flat) = tr⋅flt/gain + b                               (6)
```

with `flt` the flat level in incident photons per pixel per frame.  This leads
to:

```
tr = gain⋅(mf - b)/flt
```

However `flt` is unknown (apart from the fact that it is uniform or smooth)
so we can only assume that:

```
flt ≈ α⋅mdl                                                  (7)
```

where `mdl` is a uniform or smooth model which fits the shape of the *flat*
illumination up to an arbitrary uniform factor `α`.  Hence:

```
gain⋅(mf - b)/mdl ≈ α⋅tr
```

which yields the throughput up to an unknown factor `α`.

If we take:

```
a = mdl/(mf - b) ≈ gain/α⋅tr                                 (8)
```

and apply the affine correction:

```
img = a⋅(raw - b)                                            (9)
```

where parameter `b` is given by Eq. (1).  Then:

```
  E(img) = fg/α                                              (10)
```

which means that `img` in Eq. (9) is an estimator of the incoming flux `fg` in
some arbitrary (but constant) units.  The variance of this estimator is:

```
Var(img) = a²⋅Var(raw) = a²⋅(tr⋅fg/gain² + σ²)
         = (a/gain)⋅E(img) + (a⋅σ)²
         ≈ (a/gain)⋅[max(img, 0) + a⋅gain⋅σ²]
         ≈ [max(img,0) + v]/u                                (11)
```

using `E(img) ≈ max(img, 0)` and with:

```
u = gain/a                                                   (12)
v = a⋅gain⋅σ²                                                (13)
```

where `gain` is given in Eq. (5), `a` is given in Eq. (8) and `σ²` is given in
Eq. (2).  The approximation of the variance in Eq. (11) corresponds to weights:

```
Wgt(img) = 1/Var(img)
         ≈ u/(max(img,0) + v)                                (14)
```

To summarize, the estimators of the illumination (up to some unknown factor)
and its precision are given by:

```
     img = a⋅(raw - b)
Wgt(img) = u/(max(img,0) + v)
```

where the detector calibration parameters are:

```
a = mdl/(mf - b)
b = md
u = gain/a
v = a⋅gain⋅σ²
```

with

```
  σ² = vd
gain = (ms - md)/(vs - vd)
```
