Thinithm is drawn using OpenSCAD. This is a free program that allows models to be created using code.  
Apologies for the poor code quality. Hopefully it isn't too bad.

Most sizes can be adjusted using the Customizer panel. After changing them, press Ctrl+S to see the changes.
Always make sure there is sufficient clearance for parts, especially the micro USB connector.  
- These can be seen in the 3D preview if you move the camera inside the model.

When ready to create files for laser cutting, you need to comment out `slider_3d();` and uncomment `//slider_2d();` at
the bottom of the file. The resulting "projection" can be saved as DXF or SVG for separation into separate files or
further modifications.  
The 2d projection will generate faster if you decrease hole resolution.

Everything will need to be flipped for the top-side to be up (because I was stupid and reversed the axis).

When separating the vectors, note that the top and bottom padding of slider keys is intended to be cut from the same
material as the top layer (not the key material). This way, it will be less affected by frosting.