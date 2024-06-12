#include<stdio.h>
#include<math.h>
#include <stdint.h>


int32_t angles[] = {2949120, 1740967, 919879, 466945, 234379, 117304, 58666, 29335, 14668, 7334};



int32_t cordic_angle(int32_t x, int32_t y ) {
    int32_t x_next;
    int32_t y_next;
    int32_t angle = 0;

    //printf("\t %d, %d\n", x, y);
    
    for(int i=0; i<5; i++) {
        int32_t d = (y > 0) ? -1 : 1 ;
        if (y > 0) {
            x_next = x + (y>>i);
            y_next = y - (x>>i);
            angle += angles[i];
        } else {
            x_next = x - (y>>i);
            y_next = y + (x>>i);
            angle -= angles[i];
        }
        //printf("\t %d, %d, (%f) \n", x_next, y_next, (angle/65536.0));
        x = x_next;
        y = y_next;
    }
    return angle;
}

void main() {
    printf("Cordic\n");
    float a = 3;
    while(a<90) {
        printf("Angle of (%f): %f\n", a, cordic_angle(
                (uint32_t)(cos(a/180.0*3.14159265)*65536.0),   
                (uint32_t)(sin(a/180.0*3.14159265)*65536.0)) /65536.0);
        a+= 5.0;
    }

}