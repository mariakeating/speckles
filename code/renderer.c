#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#pragma clang diagnostic pop

#include <stdint.h>
#include <stdlib.h>

typedef struct v2_float
{
    float x;
    float y;
} v2_float;

#include "test_data.h"

#define FIXED_SHIFT 8
#define FIXED_ONE (1 << FIXED_SHIFT)
#define FIXED_MASK (FIXED_ONE - 1)

typedef struct v2_fixed
{
    int32_t x;
    int32_t y;
} v2_fixed;

#define SubV2(a, b) (v2_fixed){(a).x - (b).x, (a).y - (b).y}
#define CrossV2(a, b) (int32_t)(((int64_t)(a).x*(int64_t)(b).y - (int64_t)(a).y*(int64_t)(b).x) >> FIXED_SHIFT)

#define Minimum(a, b) ((a) < (b) ? (a) : (b))
#define Maximum(a, b) ((a) > (b) ? (a) : (b))

int
main(void)
{
    int32_t Width = 200;
    int32_t Height = 150;
    int32_t Pitch = Width*4;
    void *Memory = malloc((uint32_t)(Pitch*Height));
    
    for(int32_t P = 0;
        P < Width*Height;
        ++P)
    {
        ((uint32_t *)Memory)[P] = 0xFFFF00FF;
    }
    
    for(uint32_t TriangleIndex = 0;
        TriangleIndex < sizeof(RawIndices)/(sizeof(int32_t)*3);
        ++TriangleIndex)
    {
        v2_float Points[3] =
        {
            RawPoints[RawIndices[TriangleIndex*3 + 0] - 1],
            RawPoints[RawIndices[TriangleIndex*3 + 1] - 1],
            RawPoints[RawIndices[TriangleIndex*3 + 2] - 1],
        };
        
        for(int32_t Point32_tIndex = 0;
            Point32_tIndex < 3;
            ++Point32_tIndex)
        {
            v2_float *P = Points + Point32_tIndex;
            
            float Angle = 1.23456789f;
            float SavedX = P->x;
            float SavedY = P->y;
            
            P->x = cosf(Angle)*SavedX - sinf(Angle)*SavedY;
            P->y = sinf(Angle)*SavedX + cosf(Angle)*SavedY;
            
            P->x = P->x*(float)Width/2 + (float)Width/2;
            P->y = -P->y*(float)Height/2 + (float)Height/2;
        }
        
        v2_fixed A = (v2_fixed){(int32_t)roundf(Points[0].x*FIXED_ONE), (int32_t)roundf(Points[0].y*FIXED_ONE)};
        v2_fixed B = (v2_fixed){(int32_t)roundf(Points[1].x*FIXED_ONE), (int32_t)roundf(Points[1].y*FIXED_ONE)};
        v2_fixed C = (v2_fixed){(int32_t)roundf(Points[2].x*FIXED_ONE), (int32_t)roundf(Points[2].y*FIXED_ONE)};
        
        int32_t MinX = (Minimum(A.x, Minimum(B.x, C.x))) >> FIXED_SHIFT;
        int32_t MinY = (Minimum(A.y, Minimum(B.y, C.y))) >> FIXED_SHIFT;
        int32_t MaxX = (Maximum(A.x, Maximum(B.x, C.x)) + FIXED_MASK) >> FIXED_SHIFT;
        int32_t MaxY = (Maximum(A.y, Maximum(B.y, C.y)) + FIXED_MASK) >> FIXED_SHIFT;
        
        MinX = Maximum(MinX, 0);
        MinY = Maximum(MinY, 0);
        
        MaxX = Minimum(MaxX, Width - 1);
        MaxY = Minimum(MaxY, Height - 1);
        
        v2_fixed MinP = (v2_fixed){(MinX << FIXED_SHIFT) + FIXED_ONE/2, (MinY << FIXED_SHIFT) + FIXED_ONE/2};
        
        v2_fixed AToB = SubV2(B, A);
        v2_fixed BToC = SubV2(C, B);
        v2_fixed CToA = SubV2(A, C);
        
        v2_fixed AToMinP = SubV2(MinP, A);
        v2_fixed BToMinP = SubV2(MinP, B);
        v2_fixed CToMinP = SubV2(MinP, C);
        
        int32_t Bias0 = ((BToC.y == 0 && BToC.x < 0) || BToC.y > 0) ? 0 : -1;
        int32_t Bias1 = ((CToA.y == 0 && CToA.x < 0) || CToA.y > 0) ? 0 : -1;
        int32_t Bias2 = ((AToB.y == 0 && AToB.x < 0) || AToB.y > 0) ? 0 : -1;
        
        int32_t Area0Row = CrossV2(BToC, BToMinP) + Bias0;
        int32_t Area1Row = CrossV2(CToA, CToMinP) + Bias1;
        int32_t Area2Row = CrossV2(AToB, AToMinP) + Bias2;
        
        int32_t Area0RowInc = BToC.x;
        int32_t Area1RowInc = CToA.x;
        int32_t Area2RowInc = AToB.x;
        
        int32_t Area0Inc = -BToC.y;
        int32_t Area1Inc = -CToA.y;
        int32_t Area2Inc = -AToB.y;
        
        unsigned char *Row = (unsigned char *)Memory + MinY*Pitch + MinX*4;
        for(int32_t Y = MinY;
            Y <= MaxY;
            ++Y)
        {
            int32_t Area0 = Area0Row;
            int32_t Area1 = Area1Row;
            int32_t Area2 = Area2Row;
            
            uint32_t *Pixel = (uint32_t *)Row;
            for(int32_t X = MinX;
                X <= MaxX;
                ++X)
            {
                if(Area0 >= 0 &&
                   Area1 >= 0 &&
                   Area2 >= 0)
                {
                    assert(*Pixel == 0xFFFF00FF);
                    *Pixel = 0xFF000000;
                }
                
                Area0 += Area0Inc;
                Area1 += Area1Inc;
                Area2 += Area2Inc;
                
                ++Pixel;
            }
            
            Area0Row += Area0RowInc;
            Area1Row += Area1RowInc;
            Area2Row += Area2RowInc;
            
            Row += Pitch;
        }
    }
    
    stbi_write_png("output.png", Width, Height, 4, Memory, Pitch);
    
    return 0;
}
