Thinithm is drawn using OpenSCAD. This is a free program that allows models to be created using code.  
Apologies for the poor code quality. Hopefully it isn't too bad.

Most sizes can be adjusted using the Customizer panel. After changing them, press Ctrl+S to see the changes.
Always make sure there is sufficient clearance for parts, especially the micro USB connector.  
- These can be seen in the 3D preview if you move the camera inside the model.

Note that due to the design of the air arms, adjusting the slider bevels or making large adjustments to the slider
height may require substantial changes. Ensuring the holes in the arm bases line up with corresponding slots is very
crucial. (search for `ARM BASE HOLES` in the source to fudge these a bit)

To generate 2d files for cutting, you need to comment out `slider_3d();` and uncomment the `//slider_2d_***();` lines at
the bottom of the file (inside the projection block) one-by-one. The resulting projections can be saved as DXF or SVG.  
Projections will be faster if hole resolution is decreased.

To preview the 2d design without exporting, you can use `slider_2d_full();` instead, which is a composite of all 2d
modules. It's actually 3d because that renders faster than making a projection.