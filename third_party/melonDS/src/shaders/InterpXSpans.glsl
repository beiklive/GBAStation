#version 460

#cmakedefine ZBuffer
#cmakedefine WBuffer

layout (local_size_x = 32) in;

const uint YSpanSetup_Linear = 1U << 0;

struct YSpanSetup
{
    // Interpolator
    int XRecip;
    uint Flags;

    int X0, X1, Y0, Y1;

    uint W0n, W0d, W1d;

    // Attributes
    int Z0, Z1, W0, W1;
    int ColorR0, ColorG0, ColorB0, ColorR1, ColorG1, ColorB1;
    int TexcoordU0, TexcoordV0, TexcoordU1, TexcoordV1;

    // Slope
    int InitialDx, XMin, XMax, Increment, XCovIncr;
};

const uint XSpanSetup_Linear = 1U << 0;
const uint XSpanSetup_FillEdgeL = 1U << 1;
const uint XSpanSetup_FillEdgeR = 1U << 2;

struct XSpanSetup
{
    int X0, X1;
    int Z0, Z1, W0, W1;

    int ColorR0, ColorG0, ColorB0, ColorR1, ColorG1, ColorB1;
    int TexcoordU0, TexcoordV0, TexcoordU1, TexcoordV1;

    int EdgeLenL, EdgeLenR, EdgeCovL, EdgeCovR;

    int XRecip;

    uint Flags;
};

layout (binding = 0) uniform usamplerBuffer SetupIndices;

layout (std140, binding = 0) buffer YSpanSetupsUniform
{
    YSpanSetup YSpanSetups[];
};

layout (std140, binding = 1) buffer XSpanSetupsUniform
{
    XSpanSetup XSpanSetups[];
};

uint Umulh(uint a, uint b)
{
    uint low, high;
    umulExtended(a, b, high, low);
    return high;
}

const uint startTable[256] = uint[256](
    254, 252, 250, 248, 246, 244, 242, 240, 238, 236, 234, 233, 231, 229, 227, 225, 224, 222, 220, 218, 217, 215, 213, 212, 210, 208, 207, 205, 203, 202, 200, 199, 197, 195, 194, 192, 191, 189, 188, 186, 185, 183, 182, 180, 179, 178, 176, 175, 173, 172, 170, 169, 168, 166, 165, 164, 162, 161, 160, 158, 157, 156, 154, 153, 152, 151, 149, 148, 147, 146, 144, 143, 142, 141, 139, 138, 137, 136, 135, 134, 132, 131, 130, 129, 128, 127, 126, 125, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 108, 107, 106, 105, 104, 103, 
    102, 101, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 88, 87, 86, 85, 84, 83, 82, 81, 80, 80, 79, 78, 77, 76, 75, 74, 74, 73, 72, 71, 70, 70, 69, 68, 67, 66, 66, 65, 64, 63, 62, 62, 61, 60, 59, 59, 58, 57, 56, 56, 55, 54, 53, 53, 52, 51, 50, 50, 49, 48, 48, 47, 46, 46, 45, 44, 43, 43, 42, 41, 41, 40, 39, 39, 38, 37, 37, 36, 35, 35, 34, 33, 33, 32, 32, 31, 30, 30, 29, 28, 28, 27, 27, 26, 25, 25, 24, 24, 23, 22, 22, 21, 21, 20, 19, 19, 18, 18, 17, 17, 16, 15, 15, 14, 14, 13, 13, 12, 12, 11, 10, 10, 9, 9, 8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0
);

uint Div(uint x, uint y)
{
    // https://www.microsoft.com/en-us/research/publication/software-integer-division/
    uint k = 31 - findMSB(y);
    uint ty = (y << k) >> (32 - 9);
    uint t = startTable[ty - 256] + 256;
    uint z = (t << (32 - 9)) >> (32 - k - 1);
    uint my = 0 - z;

    z += Umulh(z, my * z);
    z += Umulh(z, my * z);

    uint q = Umulh(x, z);
    uint r = x - y * q;
    if(r >= y)
    {
        r = r - y;
        q = q + 1;
        if(r >= y)
        {
            r = r - y;
            q = q + 1;
        }
    }

    return q;
}

const int Shift = 9;

int InterpolatePersp(int yfactor, bool pidentical, int y0, int y1)
{
    if (pidentical || y0 == y1) return y0;

    // perspective-correct approx. interpolation
    if (y0 < y1)
        return y0 + (((y1-y0) * yfactor) >> Shift);
    else
        return y1 + (((y0-y1) * ((1<<Shift)-yfactor)) >> Shift);
}

int InterpolateLinear()
{

}

/*int InterpolateZ(int yfactor, int y0, int y1)
{

}*/

int CalcYFactor(YSpanSetup span, int px, int py, out bool pidentical)
{
    bool xMajor = span.Increment > 0x40000;

    int p = xMajor ? px : py;
    int p0 = xMajor ? (span.X0 - 1) : span.Y0;
    int p1 = xMajor ? (span.X1 - 1) : span.Y1;
    p -= p0;

    if (p0 != p1)
    {
        pidentical = false;

        uint num = (uint(p) * span.W0n) << Shift;
        uint den = (uint(p) * span.W0d) + (uint(p1 - p0 - p) * span.W1d);

        if (den == 0)
            return 0;
        else
            return int(Div(num, den));
    }
    else
    {
        pidentical = true;
        return 0;
    }
}

void CalcEdgeParams(YSpanSetup span, int dx, bool right, bool backwards, out int len, out int coverage)
{
    if ((span.Increment <= 0x40000) || backwards)
    {
        // Y major or backwards edges
        len = 1;

        if (span.Increment == 0)
        {
            coverage = 31;
        }
        else
        {
            int cov = ((dx >> 9) + (span.Increment >> 10)) >> 4;
            if ((cov >> 5) != (dx >> 18)) cov = 31;
            cov &= 0x1F;
            if (right == (span.X1 < span.X0))
                cov = 0x1F - cov;

            coverage = cov;
        }
    }
    else
    {
        // X major

        int lenTmp;

        if (right != (span.X1 < span.X0))
            lenTmp = (dx >> 18) - ((dx-span.Increment) >> 18);
        else
            lenTmp = ((dx+span.Increment) >> 18) - (dx >> 18);
        len = lenTmp;
        
        int xlen = span.XMax + 1 - span.XMin;
        int ylen = span.Y1 - span.Y0;
        int startx = dx >> 18;
        if (span.X1 < span.X0)
            startx = xlen - startx;
        if (right)
            startx -= lenTmp + 1;

        int startcov = int(Div(uint((startx << 10) + 0x1FF) * ylen), uint(xlen)));
        coverage = (1 << 31) | ((startcov & 0x3FF) << 12) | span.XCovIncr;
    }
}

int CalculateDx(int y, YSpanSetup span)
{
    return span.InitialDx + (y - span.Y0) * Increment;
}

int CalculateX(int dx, YSpanSetup span)
{
    int x = span.X0;
    if (span.X1 < span.X0)
        x -= dx >> 18;
    else
        x += dx >> 18;
    return clamp(x, span.XMin, span.XMax);
}

void main()
{
    uvec4 setup = texelFetch(SetupIndices, int(gl_GlobalInvocationID.x));

    YSpanSetup spanL = YSpanSetups[setup.y];
    YSpanSetup spanR = YSpanSetups[setup.z];

    int y = int(setup.w);

    int dxl = CalculateDx(y, spanL);
    int dxr = CalculateDx(y, spanR);

    int xstart = CalculateX(dxl, spanL);
    int xend = CalculateX(dxr, spanR);

    bool backwards = false;

    if (xspan.XStart > xspan.XEnd)
    {
        YSpanSetup tmpSpan = spanL;
        spanL = spanR;
        spanR = tmpSpan;

        int tmp = xstart;
        xstart = xend;
        xend = xstart;

        backwards = true;
    }

    xspan.X0 = xstart;
    xspan.X1 = xend + 1;

    xspan.XRecip = Div(1U << 30, uint(xspan.X1 - xspan.X0));

    CalcEdgeParams(spanL, dxl, false, backwards, xspan.EdgeLenL, xspan.EdgeCovL);
    CalcEdgeParams(spanR, dxr, true, backwards, xspan.EdgeLenR, xspan.EdgeCovR);

    if ((spanL.Flags & YSpanSetup_Linear) == 0U)
    {
        bool pidentical;
        int yfactor = CalcYFactor(spanL, xstart, y, pidentical);
    
        xspan.W0 = InterpolatePersp(yfactor, pidentical, spanL.W0, spanL.W1);

        xspan.ColorR0 = InterpolatePersp(yfactor, pidentical, spanL.ColorR0, spanL.ColorR1);
        xspan.ColorG0 = InterpolatePersp(yfactor, pidentical, spanL.ColorG0, spanL.ColorG1);
        xspan.ColorB0 = InterpolatePersp(yfactor, pidentical, spanL.ColorB0, spanL.ColorB1);

        xspan.TexcoordU0 = InterpolatePersp(yfactor, pidentical, spanL.TexcoordU0, spanL.TexcoordU1);
        xspan.TexcoordV0 = InterpolatePersp(yfactor, pidentical, spanL.TexcoordV0, spanL.TexcoordV1);
    }
    else
    {
    }

    if ((spanR.Flags & YSpanSetup_Linear) == 0U)
    {
        bool pidentical;
        int yfactor = CalcYFactor(spanR, xend, y, pidentical);
    
        xspan.W1 = InterpolatePersp(yfactor, pidentical, xend, spanR.W1);

        xspan.ColorR1 = InterpolatePersp(yfactor, pidentical, spanR.ColorR0, spanR.ColorR1);
        xspan.ColorG1 = InterpolatePersp(yfactor, pidentical, spanR.ColorG0, spanR.ColorG1);
        xspan.ColorB1 = InterpolatePersp(yfactor, pidentical, spanR.ColorB0, spanR.ColorB1);

        xspan.TexcoordU1 = InterpolatePersp(yfactor, pidentical, spanR.TexcoordU0, spanR.TexcoordU1);
        xspan.TexcoordV1 = InterpolatePersp(yfactor, pidentical, spanR.TexcoordV0, spanR.TexcoordV1);
    }

    XSpanSetups[gl_GlobalInvocationID.x] = xspan;    
}