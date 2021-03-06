/*
 * (c) 2002 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file libavcodec/fft-test.c
 * FFT and MDCT tests.
 */

#include "libavutil/lfg.h"
#include "dsputil.h"
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>

#undef exit

/* reference fft */

#define MUL16(a,b) ((a) * (b))

#define CMAC(pre, pim, are, aim, bre, bim) \
{\
   pre += (MUL16(are, bre) - MUL16(aim, bim));\
   pim += (MUL16(are, bim) + MUL16(bre, aim));\
}

FFTComplex *exptab;

static void fft_ref_init(int nbits, int inverse)
{
    int n, i;
    double c1, s1, alpha;

    n = 1 << nbits;
    exptab = av_malloc((n / 2) * sizeof(FFTComplex));

    for(i=0;i<(n/2);i++) {
        alpha = 2 * M_PI * (float)i / (float)n;
        c1 = cos(alpha);
        s1 = sin(alpha);
        if (!inverse)
            s1 = -s1;
        exptab[i].re = c1;
        exptab[i].im = s1;
    }
}

static void fft_ref(FFTComplex *tabr, FFTComplex *tab, int nbits)
{
    int n, i, j, k, n2;
    double tmp_re, tmp_im, s, c;
    FFTComplex *q;

    n = 1 << nbits;
    n2 = n >> 1;
    for(i=0;i<n;i++) {
        tmp_re = 0;
        tmp_im = 0;
        q = tab;
        for(j=0;j<n;j++) {
            k = (i * j) & (n - 1);
            if (k >= n2) {
                c = -exptab[k - n2].re;
                s = -exptab[k - n2].im;
            } else {
                c = exptab[k].re;
                s = exptab[k].im;
            }
            CMAC(tmp_re, tmp_im, c, s, q->re, q->im);
            q++;
        }
        tabr[i].re = tmp_re;
        tabr[i].im = tmp_im;
    }
}

static void imdct_ref(float *out, float *in, int nbits)
{
    int n = 1<<nbits;
    int k, i, a;
    double sum, f;

    for(i=0;i<n;i++) {
        sum = 0;
        for(k=0;k<n/2;k++) {
            a = (2 * i + 1 + (n / 2)) * (2 * k + 1);
            f = cos(M_PI * a / (double)(2 * n));
            sum += f * in[k];
        }
        out[i] = -sum;
    }
}

/* NOTE: no normalisation by 1 / N is done */
static void mdct_ref(float *output, float *input, int nbits)
{
    int n = 1<<nbits;
    int k, i;
    double a, s;

    /* do it by hand */
    for(k=0;k<n/2;k++) {
        s = 0;
        for(i=0;i<n;i++) {
            a = (2*M_PI*(2*i+1+n/2)*(2*k+1) / (4 * n));
            s += input[i] * cos(a);
        }
        output[k] = s;
    }
}


static float frandom(AVLFG *prng)
{
    return (int16_t)av_lfg_get(prng) / 32768.0;
}

static int64_t gettime(void)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

static void check_diff(float *tab1, float *tab2, int n, double scale)
{
    int i;
    double max= 0;
    double error= 0;

    for(i=0;i<n;i++) {
        double e= fabsf(tab1[i] - (tab2[i] / scale));
        if (e >= 1e-3) {
            av_log(NULL, AV_LOG_ERROR, "ERROR %d: %f %f\n",
                   i, tab1[i], tab2[i]);
        }
        error+= e*e;
        if(e>max) max= e;
    }
    av_log(NULL, AV_LOG_INFO, "max:%f e:%g\n", max, sqrt(error)/n);
}


static void help(void)
{
    av_log(NULL, AV_LOG_INFO,"usage: fft-test [-h] [-s] [-i] [-n b]\n"
           "-h     print this help\n"
           "-s     speed test\n"
           "-m     (I)MDCT test\n"
           "-i     inverse transform test\n"
           "-n b   set the transform size to 2^b\n"
           "-f x   set scale factor for output data of (I)MDCT to x\n"
           );
    exit(1);
}



int main(int argc, char **argv)
{
    FFTComplex *tab, *tab1, *tab_ref;
    FFTSample *tab2;
    int it, i, c;
    int do_speed = 0;
    int do_mdct = 0;
    int do_inverse = 0;
    FFTContext s1, *s = &s1;
    MDCTContext m1, *m = &m1;
    int fft_nbits, fft_size;
    double scale = 1.0;
    AVLFG prng;
    av_lfg_init(&prng, 1);

    fft_nbits = 9;
    for(;;) {
        c = getopt(argc, argv, "hsimn:f:");
        if (c == -1)
            break;
        switch(c) {
        case 'h':
            help();
            break;
        case 's':
            do_speed = 1;
            break;
        case 'i':
            do_inverse = 1;
            break;
        case 'm':
            do_mdct = 1;
            break;
        case 'n':
            fft_nbits = atoi(optarg);
            break;
        case 'f':
            scale = atof(optarg);
            break;
        }
    }

    fft_size = 1 << fft_nbits;
    tab = av_malloc(fft_size * sizeof(FFTComplex));
    tab1 = av_malloc(fft_size * sizeof(FFTComplex));
    tab_ref = av_malloc(fft_size * sizeof(FFTComplex));
    tab2 = av_malloc(fft_size * sizeof(FFTSample));

    if (do_mdct) {
        av_log(NULL, AV_LOG_INFO,"Scale factor is set to %f\n", scale);
        if (do_inverse)
            av_log(NULL, AV_LOG_INFO,"IMDCT");
        else
            av_log(NULL, AV_LOG_INFO,"MDCT");
        ff_mdct_init(m, fft_nbits, do_inverse, scale);
    } else {
        if (do_inverse)
            av_log(NULL, AV_LOG_INFO,"IFFT");
        else
            av_log(NULL, AV_LOG_INFO,"FFT");
        ff_fft_init(s, fft_nbits, do_inverse);
        fft_ref_init(fft_nbits, do_inverse);
    }
    av_log(NULL, AV_LOG_INFO," %d test\n", fft_size);

    /* generate random data */

    for(i=0;i<fft_size;i++) {
        tab1[i].re = frandom(&prng);
        tab1[i].im = frandom(&prng);
    }

    /* checking result */
    av_log(NULL, AV_LOG_INFO,"Checking...\n");

    if (do_mdct) {
        if (do_inverse) {
            imdct_ref((float *)tab_ref, (float *)tab1, fft_nbits);
            ff_imdct_calc(m, tab2, (float *)tab1);
            check_diff((float *)tab_ref, tab2, fft_size, scale);
        } else {
            mdct_ref((float *)tab_ref, (float *)tab1, fft_nbits);

            ff_mdct_calc(m, tab2, (float *)tab1);

            check_diff((float *)tab_ref, tab2, fft_size / 2, scale);
        }
    } else {
        memcpy(tab, tab1, fft_size * sizeof(FFTComplex));
        ff_fft_permute(s, tab);
        ff_fft_calc(s, tab);

        fft_ref(tab_ref, tab1, fft_nbits);
        check_diff((float *)tab_ref, (float *)tab, fft_size * 2, 1.0);
    }

    /* do a speed test */

    if (do_speed) {
        int64_t time_start, duration;
        int nb_its;

        av_log(NULL, AV_LOG_INFO,"Speed test...\n");
        /* we measure during about 1 seconds */
        nb_its = 1;
        for(;;) {
            time_start = gettime();
            for(it=0;it<nb_its;it++) {
                if (do_mdct) {
                    if (do_inverse) {
                        ff_imdct_calc(m, (float *)tab, (float *)tab1);
                    } else {
                        ff_mdct_calc(m, (float *)tab, (float *)tab1);
                    }
                } else {
                    memcpy(tab, tab1, fft_size * sizeof(FFTComplex));
                    ff_fft_calc(s, tab);
                }
            }
            duration = gettime() - time_start;
            if (duration >= 1000000)
                break;
            nb_its *= 2;
        }
        av_log(NULL, AV_LOG_INFO,"time: %0.1f us/transform [total time=%0.2f s its=%d]\n",
               (double)duration / nb_its,
               (double)duration / 1000000.0,
               nb_its);
    }

    if (do_mdct) {
        ff_mdct_end(m);
    } else {
        ff_fft_end(s);
    }
    return 0;
}
