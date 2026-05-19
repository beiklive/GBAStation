#version 460


struct XSpanSetup
{
    int X0, X1;

    int EdgeLenL, EdgeLenR, EdgeCovL, EdgeCovR;

    int XRecip;

    uint Flags;

    int Z0, Z1, W0, W1;
    int ColorR0, ColorG0, ColorB0;
    int ColorR1, ColorG1, ColorB1;
    int TexcoordU0, TexcoordV0;
    int TexcoordU1, TexcoordV1;
};

layout (std140, binding = 0) readonly buffer PolygonsUniform
{
    Polygon Polygons[];
};

layout (std140, binding = 1) readonly buffer XSpanSetupsUniform
{
    XSpanSetup XSpanSetups[];
};
