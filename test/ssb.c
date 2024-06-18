# define M_PI           3.14159265358979323846
 
#include<stdio.h>
#include<math.h>
#include <stdint.h>
#include <stdlib.h>


#define GGG 16384
#define GG2 32768

#define XSCALE 9949

#define LLL 64

typedef struct q_cmplx16 {
    int16_t real;
    int16_t imag;
} q_cmplx16;

typedef struct q_cmplx_polar_16 {
    int16_t mag;
    int16_t phase;
} q_cmplx_polar_16;


q_cmplx16 *signal;
q_cmplx16 *signal2;
q_cmplx16 *signal3;
q_cmplx16 *signal4;
q_cmplx_polar_16 *output;


typedef struct biquad {
    int16_t gain;
    int16_t a1;
    int16_t a2;

    q_cmplx16 x1;
    q_cmplx16 x2;
    q_cmplx16 y1;
    q_cmplx16 y2;
} biquad;


// degree*8
// scilab: for idx=0:10; p(idx+1) = round(atan(1/2**idx)/M_PI*180*8); end;
int16_t angles[] = { 360, 213, 112, 57, 29, 14, 7, 4, 2, 1 };
//
int16_t scale_correct = 5555;

q_cmplx16 pi4freq[] = { (q_cmplx16){GGG, 0}, (q_cmplx16){0, GGG}, (q_cmplx16){-GGG, 0}, (q_cmplx16){0, -GGG}};
q_cmplx16 pi4nfreq[] = { (q_cmplx16){GGG, 0}, (q_cmplx16){0, -GGG}, (q_cmplx16){-GGG, 0}, (q_cmplx16){0, GGG}};


biquad *pi4_filter;

int16_t qmult(int16_t a, int16_t b) {
    int32_t k = (int32_t)a * (int32_t)b;
    return (k/GGG);
}

q_cmplx16 q_cmplx_mult(q_cmplx16 a, q_cmplx16 b) {
    q_cmplx16 ret;
    ret.real = qmult(a.real, b.real) - qmult(a.imag, b.imag);
    ret.imag = qmult(a.real, b.imag) - qmult(a.imag, b.real);
    return(ret);
}

q_cmplx16 q_cmplx_add(q_cmplx16 a, q_cmplx16 b) {
    return((q_cmplx16){a.real + b.real, a.imag + b.imag});
}

void print_q_cmplx16_arr( q_cmplx16 *arr, int len) {
    printf("%c", '{');
    for(int i=0; i<len; i++) {
        printf("(%d, %d), ", arr[i].real, arr[i].imag);
    }
    printf("%c\n", '}');
}

void sci_lab_print_q_cmplx16_arr( q_cmplx16 *arr, int len) {
    printf("%c", '[');
    printf("(%d + %d *%%i)", arr[0].real, arr[0].imag);
    for(int i=1; i<len; i++) {
        printf(", (%d + %d *%%i)", arr[i].real, arr[i].imag);
    }
    printf("%c\n", ']');
}


void sci_lab_print_q_cmplx_polar_arr(q_cmplx_polar_16 *arr, int len) {
    printf("%c", '[');
    printf("[%d, %d]", arr[0].mag, arr[0].phase);
    for(int i=1; i<len; i++) {
        printf(", [%d, %d]", arr[i].mag, arr[i].phase);
    }
    printf("%c\n", ']');
}

int16_t qconv(float a) {
    return ((int16_t)(a * (float)GGG));
}

q_cmplx16 *fpi4_shift(int direction, q_cmplx16 *arr, int len ) {
    q_cmplx16* ret = (q_cmplx16*)malloc(len * sizeof(q_cmplx16));
    for(int i=0; i<len; i++) {
        q_cmplx16 e;
        if(direction) {
          e = q_cmplx_mult(arr[i], pi4freq[i & 0x3]);
          ret[i].real = e.real;
          ret[i].imag = e.imag;
        } else {
          e = q_cmplx_mult(arr[i], pi4nfreq[i & 0x3]);
          ret[i].real = e.real;
          ret[i].imag = e.imag;
        }
    }
    return(ret);
}

q_cmplx16 biquad_step(q_cmplx16 x0, biquad *sm) {
    q_cmplx16 y0 = {0,0};
    // y[n] = b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2]

    y0 = q_cmplx_mult( // gain*(b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2])
            (q_cmplx16){sm->gain, 0}, 
            q_cmplx_add( // b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2]
                q_cmplx_add( // b0 * x[n] + b1 * x[n-1] +  b2 * x[n-2] + a1 * y[n-1]
                    q_cmplx_add( // b0 * x[n] + b1 * x[n-1] +  b2 * x[n-2]
                        q_cmplx_add( // b0 * x[n] + b1 * x[n-1]
                            q_cmplx_mult((q_cmplx16){GGG, 0}, x0),
                            q_cmplx_mult((q_cmplx16){GG2, 0}, sm->x1)
                        ),
                        q_cmplx_mult((q_cmplx16){GGG, 0}, sm->x2)
                    ),
                    q_cmplx_mult((q_cmplx16){sm->a1, 0}, sm->y1)
                ),
                q_cmplx_mult((q_cmplx16){sm->a2, 0}, sm->y2)
            )
    );
    sm->x2 = sm->x1;
    sm->x1 = x0;
    sm->y2 = sm->y1;
    sm->y1 = y0;
    return(y0);
}

q_cmplx16 biquad_cascade_step(q_cmplx16 x0, biquad *sm, int len) {
    q_cmplx16 casc;
    casc.real = x0.real;
    casc.imag = x0.imag;
    for(int i=0; i<len; i++) {
        q_cmplx16 out = biquad_step(casc, &sm[i]);
        casc.real = out.real;
        casc.imag = out.imag;
    }
    return(casc);
}

void biquad_cascade_pi4_filter_init() {
    pi4_filter = (biquad*)malloc(2* sizeof(biquad));
    pi4_filter[0] =  (biquad) {qconv(0.2599),0, qconv(0.0396), (q_cmplx16){0, 0}, (q_cmplx16){0, 0}, (q_cmplx16){0, 0}, (q_cmplx16){0, 0}};
    pi4_filter[1] =  (biquad) {qconv(0.3616),0, qconv(0.4465), (q_cmplx16){0, 0}, (q_cmplx16){0, 0}, (q_cmplx16){0, 0}, (q_cmplx16){0, 0}};
}

q_cmplx16 *generate_sample(int len) {
  q_cmplx16 *sample = (q_cmplx16*)malloc(len * sizeof(q_cmplx16));
  for(int i=0; i<len; i++){
    sample[i].real = qconv( (
                    0.2*sin((2*M_PI* 50.0*(float)i)/1000.0         ) + 
                    0.4*sin((2*M_PI*100.0*(float)i)/1000.0+M_PI/4.0) +   
                    0.5*sin((2*M_PI*150.0*(float)i)/1000.0+M_PI/4.0) + 
                    0.6*sin((2*M_PI*200.0*(float)i)/1000.0+M_PI/4.0) +  
                    0.7*sin((2*M_PI*250.0*(float)i)/1000.0+M_PI/4.0) + 
                    0.8*sin((2*M_PI*300.0*(float)i)/1000.0+M_PI/4.0) +  
                    0.9*sin((2*M_PI*350.0*(float)i)/1000.0+M_PI/4.0) + 
                    0.2*sin((2*M_PI*400.0*(float)i)/1000.0+M_PI/4.0)) / 3.0 );
    sample[i].imag = 0;
  }
  return sample;
}

q_cmplx16 *do_filter(q_cmplx16 *in_s, int len ) {
    q_cmplx16 *result = (q_cmplx16*)malloc(len * sizeof(q_cmplx16));
    for(int i=0; i<len; i++) {
        q_cmplx16 r = biquad_cascade_step(in_s[i], pi4_filter, 2);
        result[i].real = r.real;
        result[i].imag = r.imag;
    }
    return result;
}

q_cmplx_polar_16 cordic_angle(int16_t x, int16_t y ) {
    int16_t x_next;
    int16_t y_next;
    int16_t angle = 0;
    int quadrant;

    if ( (x>=0) && (y>=0)) {
        quadrant = 0;
    } else if ( (x<0) && (y>=0) ) {
        quadrant = 1;
        y_next = y;
        y = -1 * x;
        x = y_next;
    } else if ( (x<0) && (y<0) ) {
        quadrant = 2;
        y = -1 * y;
        x = -1 * x;
    } else {
        quadrant = 3;
        x_next = x;
        x = -1 * y;
        y = x_next;
    }
    
    for(int i=0; i<10; i++) {
        int16_t d = (y > 0) ? -1 : 1 ;
        if (y > 0) {
            x_next = x + (y>>i);
            y_next = y - (x>>i);
            angle += angles[i];
        } else {
            x_next = x - (y>>i);
            y_next = y + (x>>i);
            angle -= angles[i];
        }
        x = x_next;
        y = y_next;
    }
    switch (quadrant)
    {
    case 0:
        return ((q_cmplx_polar_16){qmult(x_next,XSCALE), angle }) ;
    case 1: 
        return ((q_cmplx_polar_16){qmult(x_next,XSCALE), angle +720});
    case 2: 
        return ((q_cmplx_polar_16){qmult(x_next,XSCALE), angle +1440});
    case 3: 
        return ((q_cmplx_polar_16){qmult(x_next,XSCALE), angle +2160});    
    }
}

q_cmplx_polar_16 *generate_output(q_cmplx16 *in_s, int len) {
    q_cmplx_polar_16 *result = (q_cmplx_polar_16*)malloc(len * sizeof(q_cmplx_polar_16));
    for(int i=0; i<len; i++) {
        q_cmplx_polar_16 z = cordic_angle(in_s[i].real, in_s[i].imag);
        result[i].mag = z.mag;
        result[i].phase = z.phase;
    }
    return result;
}

void main() {
    signal = generate_sample(LLL);
    sci_lab_print_q_cmplx16_arr(signal, LLL);

    signal2 = fpi4_shift(1, signal, LLL);
    sci_lab_print_q_cmplx16_arr(signal2, LLL);


    biquad_cascade_pi4_filter_init();
    signal3 = do_filter(signal2, LLL);
    sci_lab_print_q_cmplx16_arr(signal3, LLL);

    signal4 = fpi4_shift(0, signal3, LLL);
    sci_lab_print_q_cmplx16_arr(signal4, LLL);

    output = generate_output(signal4, LLL);
    sci_lab_print_q_cmplx_polar_arr(output, LLL);



    
    printf("\n\nCordic\n");
    float a = 3;
    while(a<360) {
        float g = 0.2*cos(a/180.0*3.14159265);
        float h = 0.7*sin(a/180.0*3.14159265);
        q_cmplx_polar_16 z = cordic_angle(
                (uint16_t)(g*GGG),   
                (uint16_t)(h*GGG));

        //printf("Angle of (%f): %f\n", a, (float)z.phase/8.0);
        printf("\tScale of (%f): %f\n", sqrt(g*g+h*h), (float)z.mag/(float)GGG);
        a+= 5.0;
    }

}