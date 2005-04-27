/*!
 * \file vdr_sound.c
 * \brief Sound manipulation classes for a VDR media plugin (muggle)
 *
 * \version $Revision: 1.2 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author$
 *
 * $Id$
 *
 * Adapted from
 * MP3/MPlayer plugin to VDR (C++)
 * (C) 2001,2002 Stefan Huelswitt <huels@iname.com>
 */

// --- cResample ------------------------------------------------------------

// The resample code has been adapted from the madplay project
// (resample.c) found in the libmad distribution

class cResample
{
    private:
        mad_fixed_t ratio;
        mad_fixed_t step;
        mad_fixed_t last;
        mad_fixed_t resampled[MAX_NSAMPLES];
    public:
        bool SetInputRate (unsigned int oldrate, unsigned int newrate);
        unsigned int ResampleBlock (unsigned int nsamples, const mad_fixed_t * old);
        const mad_fixed_t *Resampled (void)
        {
            return resampled;
        }
};

bool cResample::SetInputRate (unsigned int oldrate, unsigned int newrate)
{
    if (oldrate < 8000 || oldrate > newrate * 6)
    {                                             // out of range
        esyslog ("WARNING: samplerate %d out of range 8000-%d\n", oldrate,
            newrate * 6);
        return 0;
    }
    ratio = mad_f_tofixed ((double) oldrate / (double) newrate);
    step = 0;
    last = 0;
#ifdef DEBUG
    static mad_fixed_t
        oldratio = 0;
    if (oldratio != ratio)
    {
        printf ("mad: new resample ratio %f (from %d kHz to %d kHz)\n",
            mad_f_todouble (ratio), oldrate, newrate);
        oldratio = ratio;
    }
#endif
    return ratio != MAD_F_ONE;
}


unsigned int
cResample::ResampleBlock (unsigned int nsamples, const mad_fixed_t * old)
{
// This resampling algorithm is based on a linear interpolation, which is
// not at all the best sounding but is relatively fast and efficient.
//
// A better algorithm would be one that implements a bandlimited
// interpolation.

    mad_fixed_t *nsam = resampled;
    const mad_fixed_t *end = old + nsamples;
    const mad_fixed_t *begin = nsam;

    if (step < 0)
    {
        step = mad_f_fracpart (-step);

        while (step < MAD_F_ONE)
        {
            *nsam++ = step ? last + mad_f_mul (*old - last, step) : last;
            step += ratio;
            if (((step + 0x00000080L) & 0x0fffff00L) == 0)
                step = (step + 0x00000080L) & ~0x0fffffffL;
        }
        step -= MAD_F_ONE;
    }

    while (end - old > 1 + mad_f_intpart (step))
    {
        old += mad_f_intpart (step);
        step = mad_f_fracpart (step);
        *nsam++ = step ? *old + mad_f_mul (old[1] - old[0], step) : *old;
        step += ratio;
        if (((step + 0x00000080L) & 0x0fffff00L) == 0)
            step = (step + 0x00000080L) & ~0x0fffffffL;
    }

    if (end - old == 1 + mad_f_intpart (step))
    {
        last = end[-1];
        step = -step;
    }
    else
        step -= mad_f_fromint (end - old);

    return nsam - begin;
}


// --- cLevel ----------------------------------------------------------------

// The normalize algorithm and parts of the code has been adapted from the
// Normalize 0.7 project. (C) 1999-2002, Chris Vaill <cvaill@cs.columbia.edu>

// A little background on how normalize computes the volume
// of a wav file, in case you want to know just how your
// files are being munged:
//
// The volumes calculated are RMS amplitudes, which corre­
// spond (roughly) to perceived volume. Taking the RMS ampli­
// tude of an entire file would not give us quite the measure
// we want, though, because a quiet song punctuated by short
// loud parts would average out to a quiet song, and the
// adjustment we would compute would make the loud parts
// excessively loud.
//
// What we want is to consider the maximum volume of the
// file, and normalize according to that. We break up the
// signal into 100 chunks per second, and get the signal
// power of each chunk, in order to get an estimation of
// "instantaneous power" over time. This "instantaneous
// power" signal varies too much to get a good measure of the
// original signal's maximum sustained power, so we run a
// smoothing algorithm over the power signal (specifically, a
// mean filter with a window width of 100 elements). The max­
// imum point of the smoothed power signal turns out to be a
// good measure of the maximum sustained power of the file.
// We can then take the square root of the power to get maxi­
// mum sustained RMS amplitude.

class cLevel
{
    private:
        double maxpow;
        mad_fixed_t peak;
        struct Power
        {
// smooth
            int npow, wpow;
            double powsum, pows[POW_WIN];
// sum
            unsigned int nsum;
            double sum;
        } power[2];
//
        inline void AddPower (struct Power *p, double pow);
    public:
        void Init (void);
        void GetPower (struct mad_pcm *pcm);
        double GetLevel (void);
        double GetPeak (void);
};

void
cLevel::Init (void)
{
    for (int l = 0; l < 2; l++)
    {
        struct Power *p = &power[l];
        p->sum = p->powsum = 0.0;
        p->wpow = p->npow = p->nsum = 0;
        for (int i = POW_WIN - 1; i >= 0; i--)
            p->pows[i] = 0.0;
    }
    maxpow = 0.0;
    peak = 0;
}


void
cLevel::GetPower (struct mad_pcm *pcm)
{
    for (int i = 0; i < pcm->channels; i++)
    {
        struct Power *p = &power[i];
        mad_fixed_t *data = pcm->samples[i];
        for (int n = pcm->length; n > 0; n--)
        {
            if (*data < -peak)
                peak = -*data;
            if (*data > peak)
                peak = *data;
            double s = mad_f_todouble (*data++);
            p->sum += (s * s);
            if (++(p->nsum) >= pcm->samplerate / 100)
            {
                AddPower (p, p->sum / (double) p->nsum);
                p->sum = 0.0;
                p->nsum = 0;
            }
        }
    }
}


void
cLevel::AddPower (struct Power *p, double pow)
{
    p->powsum += pow;
    if (p->npow >= POW_WIN)
    {
        if (p->powsum > maxpow)
            maxpow = p->powsum;
        p->powsum -= p->pows[p->wpow];
    }
    else
        p->npow++;
    p->pows[p->wpow] = pow;
    p->wpow = (p->wpow + 1) % POW_WIN;
}


double
cLevel::GetLevel (void)
{
    if (maxpow < EPSILON)
    {
// Either this whole file has zero power, or was too short to ever
// fill the smoothing buffer.  In the latter case, we need to just
// get maxpow from whatever data we did collect.

        if (power[0].powsum > maxpow)
            maxpow = power[0].powsum;
        if (power[1].powsum > maxpow)
            maxpow = power[1].powsum;
    }
                                                  // adjust for the smoothing window size and root
    double level = sqrt (maxpow / (double) POW_WIN);
    printf ("norm: new volumen level=%f peak=%f\n", level,
        mad_f_todouble (peak));
    return level;
}


double
cLevel::GetPeak (void)
{
    return mad_f_todouble (peak);
}


// --- cNormalize ------------------------------------------------------------

class cNormalize
{
    private:
        mad_fixed_t gain;
        double d_limlvl, one_limlvl;
        mad_fixed_t limlvl;
        bool dogain, dolimit;
#ifdef DEBUG
// stats
        unsigned long limited, clipped, total;
        mad_fixed_t peak;
#endif
// limiter
#ifdef USE_FAST_LIMITER
        mad_fixed_t *table, tablestart;
        int tablesize;
        inline mad_fixed_t FastLimiter (mad_fixed_t x);
#endif
        inline mad_fixed_t Limiter (mad_fixed_t x);
    public:
        cNormalize (void);
        ~cNormalize ();
        void Init (double Level, double Peak);
        void Stats (void);
        void AddGain (struct mad_pcm *pcm);
};

cNormalize::cNormalize (void)
{
    d_limlvl = (double) the_setup.LimiterLevel / 100.0;
    one_limlvl = 1 - d_limlvl;
    limlvl = mad_f_tofixed (d_limlvl);
    printf ("norm: lim_lev=%f lim_acc=%d\n", d_limlvl, LIM_ACC);

#ifdef USE_FAST_LIMITER
    mad_fixed_t start = limlvl & ~(F_LIM_JMP - 1);
    tablestart = start;
    tablesize = (unsigned int) (F_LIM_MAX - start) / F_LIM_JMP + 2;
    table = new mad_fixed_t[tablesize];
    if (table)
    {
        printf ("norm: table size=%d start=%08x jump=%08x\n", tablesize, start,
            F_LIM_JMP);
        for (int i = 0; i < tablesize; i++)
        {
            table[i] = Limiter (start);
            start += F_LIM_JMP;
        }
        tablesize--;                              // avoid a -1 in FastLimiter()

// do a quick accuracy check, just to be sure that FastLimiter() is working
// as expected :-)
#ifdef ACC_DUMP
        FILE *out = fopen ("/tmp/limiter", "w");
#endif
        mad_fixed_t maxdiff = 0;
        for (mad_fixed_t x = F_LIM_MAX; x >= limlvl; x -= mad_f_tofixed (1e-4))
        {
            mad_fixed_t diff = mad_f_abs (Limiter (x) - FastLimiter (x));
            if (diff > maxdiff)
                maxdiff = diff;
#ifdef ACC_DUMP
            fprintf (out, "%0.10f\t%0.10f\t%0.10f\t%0.10f\t%0.10f\n",
                mad_f_todouble (x), mad_f_todouble (Limiter (x)),
                mad_f_todouble (FastLimiter (x)), mad_f_todouble (diff),
                mad_f_todouble (maxdiff));
            if (ferror (out))
                break;
#endif
        }
#ifdef ACC_DUMP
        fclose (out);
#endif
        printf ("norm: accuracy %.12f\n", mad_f_todouble (maxdiff));
        if (mad_f_todouble (maxdiff) > 1e-6)
        {
            esyslog ("ERROR: accuracy check failed, normalizer disabled");
            delete table;
            table = 0;
        }
    }
    else
        esyslog ("ERROR: no memory for lookup table, normalizer disabled");
#endif                                        // USE_FAST_LIMITER
}


cNormalize::~cNormalize ()
{
#ifdef USE_FAST_LIMITER
    delete[] table;
#endif
}


void
cNormalize::Init (double Level, double Peak)
{
    double Target = (double) the_setup.TargetLevel / 100.0;
    double dgain = Target / Level;
    if (dgain > MAX_GAIN)
        dgain = MAX_GAIN;
    gain = mad_f_tofixed (dgain);
// Check if we actually need to apply a gain
    dogain = (Target > 0.0 && fabs (1 - dgain) > MIN_GAIN);
#ifdef USE_FAST_LIMITER
    if (!table)
        dogain = false;
#endif
// Check if we actually need to do limiting:
// we have to if limiter is enabled, if gain>1 and if the peaks will clip.
    dolimit = (d_limlvl < 1.0 && dgain > 1.0 && Peak * dgain > 1.0);
#ifdef DEBUG
    printf ("norm: gain=%f dogain=%d dolimit=%d (target=%f level=%f peak=%f)\n",
        dgain, dogain, dolimit, Target, Level, Peak);
    limited = clipped = total = 0;
    peak = 0;
#endif
}


void
cNormalize::Stats (void)
{
#ifdef DEBUG
    if (total)
        printf ("norm: stats tot=%ld lim=%ld/%.3f%% clip=%ld/%.3f%% peak=%.3f\n",
            total, limited, (double) limited / total * 100.0, clipped,
            (double) clipped / total * 100.0, mad_f_todouble (peak));
#endif
}


mad_fixed_t cNormalize::Limiter (mad_fixed_t x)
{
// Limiter function:
//
//        / x                                                (for x <= lev)
//   x' = |
//        \ tanh((x - lev) / (1-lev)) * (1-lev) + lev        (for x > lev)
//
// call only with x>=0. For negative samples, preserve sign outside this function
//
// With limiter level = 0, this is equivalent to a tanh() function;
// with limiter level = 1, this is equivalent to clipping.

    if (x > limlvl)
    {
#ifdef DEBUG
        if (x > MAD_F_ONE)
            clipped++;
        limited++;
#endif
        x =
            mad_f_tofixed (tanh ((mad_f_todouble (x) - d_limlvl) / one_limlvl) *
            one_limlvl + d_limlvl);
    }
    return x;
}


#ifdef USE_FAST_LIMITER
mad_fixed_t cNormalize::FastLimiter (mad_fixed_t x)
{
// The fast algorithm is based on a linear interpolation between the
// the values in the lookup table. Relays heavly on libmads fixed point format.

    if (x > limlvl)
    {
        int
            i = (unsigned int) (x - tablestart) / F_LIM_JMP;
#ifdef DEBUG
        if (x > MAD_F_ONE)
            clipped++;
        limited++;
        if (i >= tablesize)
            printf ("norm: overflow x=%f x-ts=%f i=%d tsize=%d\n",
                mad_f_todouble (x), mad_f_todouble (x - tablestart), i,
                tablesize);
#endif
        mad_fixed_t
            r = x & (F_LIM_JMP - 1);
        x = MAD_F_ONE;
        if (i < tablesize)
        {
            mad_fixed_t *
                ptr = &table[i];
            x = *ptr;
            mad_fixed_t
                d = *(ptr + 1) - x;
//x+=mad_f_mul(d,r)<<LIM_ACC;                // this is not accurate as mad_f_mul() does >>MAD_F_FRACBITS
// which is senseless in the case of following <<LIM_ACC.
                                                  // better, don't know if works on all machines
            x += ((long long) d * (long long) r) >> LIM_SHIFT;
        }
    }
    return x;
}
#endif

#ifdef USE_FAST_LIMITER
#define LIMITER_FUNC FastLimiter
#else
#define LIMITER_FUNC Limiter
#endif

void
cNormalize::AddGain (struct mad_pcm *pcm)
{
    if (dogain)
    {
        for (int i = 0; i < pcm->channels; i++)
        {
            mad_fixed_t *data = pcm->samples[i];
#ifdef DEBUG
            total += pcm->length;
#endif
            if (dolimit)
            {
                for (int n = pcm->length; n > 0; n--)
                {
                    mad_fixed_t s = mad_f_mul (*data, gain);
                    if (s < 0)
                    {
                        s = -s;
#ifdef DEBUG
                        if (s > peak)
                            peak = s;
#endif
                        s = LIMITER_FUNC (s);
                        s = -s;
                    }
                    else
                    {
#ifdef DEBUG
                        if (s > peak)
                            peak = s;
#endif
                        s = LIMITER_FUNC (s);
                    }
                    *data++ = s;
                }
            }
            else
            {
                for (int n = pcm->length; n > 0; n--)
                {
                    mad_fixed_t s = mad_f_mul (*data, gain);
#ifdef DEBUG
                    if (s > peak)
                        peak = s;
                    else if (-s > peak)
                        peak = -s;
#endif
                    if (s > MAD_F_ONE)
                        s = MAD_F_ONE;            // do clipping
                    if (s < -MAD_F_ONE)
                        s = -MAD_F_ONE;
                    *data++ = s;
                }
            }
        }
    }
}


// --- cScale ----------------------------------------------------------------

// The dither code has been adapted from the madplay project
// (audio.c) found in the libmad distribution

enum eAudioMode
{ amRound, amDither };

class cScale
{
    private:
        enum
        { MIN = -MAD_F_ONE, MAX = MAD_F_ONE - 1 };
#ifdef DEBUG
// audio stats
        unsigned long clipped_samples;
        mad_fixed_t peak_clipping;
        mad_fixed_t peak_sample;
#endif
// dither
        struct dither
        {
            mad_fixed_t error[3];
            mad_fixed_t random;
        } leftD, rightD;
//
        inline mad_fixed_t Clip (mad_fixed_t sample, bool stats = true);
        inline signed long LinearRound (mad_fixed_t sample);
        inline unsigned long Prng (unsigned long state);
        inline signed long LinearDither (mad_fixed_t sample, struct dither *dither);
    public:
	cScale();
        void Stats (void);
        unsigned int ScaleBlock (unsigned char *data, unsigned int size,
            unsigned int &nsamples, const mad_fixed_t * &left,
            const mad_fixed_t * &right, eAudioMode mode);
};

cScale::cScale()
{
#ifdef DEBUG
    clipped_samples = 0;
    peak_clipping = peak_sample = 0;
#endif
    memset (&leftD, 0, sizeof (leftD));
    memset (&rightD, 0, sizeof (rightD));
}


void
cScale::Stats (void)
{
#ifdef DEBUG
    printf ("mp3: scale stats clipped=%ld peak_clip=%f peak=%f\n",
        clipped_samples, mad_f_todouble (peak_clipping),
        mad_f_todouble (peak_sample));
#endif
}


// gather signal statistics while clipping
mad_fixed_t cScale::Clip (mad_fixed_t sample, bool stats)
{
#ifndef DEBUG
    if (sample > MAX)
        sample = MAX;
    if (sample < MIN)
        sample = MIN;
#else
    if (!stats)
    {
        if (sample > MAX)
            sample = MAX;
        if (sample < MIN)
            sample = MIN;
    }
    else
    {
        if (sample >= peak_sample)
        {
            if (sample > MAX)
            {
                ++clipped_samples;
                if (sample - MAX > peak_clipping)
                    peak_clipping = sample - MAX;
                sample = MAX;
            }
            peak_sample = sample;
        }
        else if (sample < -peak_sample)
        {
            if (sample < MIN)
            {
                ++clipped_samples;
                if (MIN - sample > peak_clipping)
                    peak_clipping = MIN - sample;
                sample = MIN;
            }
            peak_sample = -sample;
        }
    }
#endif
    return sample;
}


// generic linear sample quantize routine
signed long
cScale::LinearRound (mad_fixed_t sample)
{
// round
    sample += (1L << (MAD_F_FRACBITS - OUT_BITS));
// clip
    sample = Clip (sample);
// quantize and scale
    return sample >> (MAD_F_FRACBITS + 1 - OUT_BITS);
}


// 32-bit pseudo-random number generator
unsigned long
cScale::Prng (unsigned long state)
{
    return (state * 0x0019660dL + 0x3c6ef35fL) & 0xffffffffL;
}


// generic linear sample quantize and dither routine
signed long
cScale::LinearDither (mad_fixed_t sample, struct dither *dither)
{
    unsigned int scalebits;
    mad_fixed_t output, mask, random;

// noise shape
    sample += dither->error[0] - dither->error[1] + dither->error[2];
    dither->error[2] = dither->error[1];
    dither->error[1] = dither->error[0] / 2;
// bias
    output = sample + (1L << (MAD_F_FRACBITS + 1 - OUT_BITS - 1));
    scalebits = MAD_F_FRACBITS + 1 - OUT_BITS;
    mask = (1L << scalebits) - 1;
// dither
    random = Prng (dither->random);
    output += (random & mask) - (dither->random & mask);
    dither->random = random;
// clip
    output = Clip (output);
    sample = Clip (sample, false);
// quantize
    output &= ~mask;
// error feedback
    dither->error[0] = sample - output;
// scale
    return output >> scalebits;
}


// write a block of signed 16-bit big-endian PCM samples
unsigned int
cScale::ScaleBlock (unsigned char *data, unsigned int size,
unsigned int &nsamples, const mad_fixed_t * &left,
const mad_fixed_t * &right, eAudioMode mode)
{
    signed int sample;
    unsigned int len, res;

    len = size / OUT_FACT;
    res = size;
    if (len > nsamples)
    {
        len = nsamples;
        res = len * OUT_FACT;
    }
    nsamples -= len;

    if (right)
    {                                             // stereo
        switch (mode)
        {
            case amRound:
                while (len--)
                {
                    sample = LinearRound (*left++);
                    *data++ = sample >> 8;
                    *data++ = sample >> 0;
                    sample = LinearRound (*right++);
                    *data++ = sample >> 8;
                    *data++ = sample >> 0;
                }
                break;
            case amDither:
                while (len--)
                {
                    sample = LinearDither (*left++, &leftD);
                    *data++ = sample >> 8;
                    *data++ = sample >> 0;
                    sample = LinearDither (*right++, &rightD);
                    *data++ = sample >> 8;
                    *data++ = sample >> 0;
                }
                break;
        }
    }
    else
    {                                             // mono, duplicate left channel
        switch (mode)
        {
            case amRound:
                while (len--)
                {
                    sample = LinearRound (*left++);
                    *data++ = sample >> 8;
                    *data++ = sample >> 0;
                    *data++ = sample >> 8;
                    *data++ = sample >> 0;
                }
                break;
            case amDither:
                while (len--)
                {
                    sample = LinearDither (*left++, &leftD);
                    *data++ = sample >> 8;
                    *data++ = sample >> 0;
                    *data++ = sample >> 8;
                    *data++ = sample >> 0;
                }
                break;
        }
    }
    return res;
}
