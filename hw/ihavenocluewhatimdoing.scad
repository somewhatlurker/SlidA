pcb_width_main = 97;
pcb_width_ledsoffset = 3;
pcb_width_total = pcb_width_main + pcb_width_ledsoffset;
pcb_height_main = 85.7;
pcb_height_leds = 8.1;
pcb_height_total = pcb_height_main + pcb_height_leds;

pcb_hole_dist_x = 6.175;
pcb_hole_dist_y = 3.175;
pcb_hole_size = 3.2;

top_thickness = 1.5;
key_thickness = 3;
pcb_thickness = 1.6;
space_thickness = 8;
bottom_thickness = 2;
wall_thickness = 2;

wall_height = key_thickness + pcb_thickness + space_thickness;
full_height = top_thickness + wall_height + bottom_thickness;

slider_height = 136;
top_width = 470;
bottom_width = 530;
slider_y_adjust = -9.5;

key_separator_width = 3;
key_top_bottom_padding = 1.5;
key_area_width = pcb_width_main * 4 + key_separator_width;
key_area_height = pcb_height_total - pcb_hole_dist_y*2 - pcb_hole_size - key_top_bottom_padding*2 - .15;

tab_tolerance = 0.1;


module slider_holes (n = 4)
{
    for (i=[0:n-1])
    {
        translate ([(-pcb_width_main/2) * n + pcb_width_main * i, -pcb_height_total/2, 0])
        {
            translate ([pcb_hole_dist_x, pcb_hole_dist_y, 0])
            {
                circle(pcb_hole_size/2, $fn=32);
            }
            translate ([pcb_width_main - pcb_hole_dist_x, pcb_hole_dist_y, 0])
            {
                circle(pcb_hole_size/2, $fn=32);
            }
            translate ([pcb_hole_dist_x, pcb_height_total - pcb_hole_dist_y, 0])
            {
                circle(pcb_hole_size/2, $fn=32);
            }
            translate ([pcb_width_main - pcb_hole_dist_x, pcb_height_total - pcb_hole_dist_y, 0])
            {
                circle(pcb_hole_size/2, $fn=32);
            }
        }
    }
    
    translate ([-key_area_width/2 - pcb_hole_size/2 - .07, 0, 0])
    {
        circle(pcb_hole_size/2, $fn=32);
    }
    translate ([key_area_width/2 + pcb_hole_size/2 + .07, 0, 0])
    {
        circle(pcb_hole_size/2, $fn=32);
    }
}

module slider_pcbs (n = 4)
{
    corner_rad = 1.27;
    
    color("white", 1.0)
    linear_extrude (height = pcb_thickness)
    difference ()
    {
        for (i=[0:n-1])
        {
            translate ([(-pcb_width_main/2) * n + pcb_width_main * i, -pcb_height_total/2])
            {
                union ()
                {
                    // main body full width (not full height)
                    translate ([0, pcb_height_leds])
                    {
                        square([pcb_width_main, pcb_height_main - corner_rad]);
                    }
                    // main body full height (not full width)
                    translate ([corner_rad, pcb_height_leds])
                    {
                        square([pcb_width_main - corner_rad * 2, pcb_height_main]);
                    }
    
                    // main body corners
                    translate ([corner_rad, pcb_height_total - corner_rad])
                    {
                        circle(r = corner_rad, $fn=32);
                    }
                    translate ([pcb_width_main - corner_rad, pcb_height_total - corner_rad])
                    {
                        circle(r = corner_rad, $fn=32);
                    }
                    
                    // led section full width (not full height)
                    translate ([pcb_width_ledsoffset, corner_rad])
                    {
                        square([pcb_width_main, pcb_height_leds - corner_rad]);
                    }
                    // led section full height (not full width)
                    translate ([pcb_width_ledsoffset + corner_rad, 0])
                    {
                        square([pcb_width_main - corner_rad * 2, pcb_height_leds]);
                    }
                    
                    // led section corners
                    translate ([pcb_width_ledsoffset + corner_rad, corner_rad])
                    {
                        circle(r = corner_rad, $fn=32);
                    }
                    translate ([pcb_width_ledsoffset + pcb_width_main - corner_rad, corner_rad])
                    {
                        circle(r = corner_rad, $fn=32);
                    }
                }
            }
        }
        slider_holes(n);
    }
}

module slider_keys (width = key_area_width, height = key_area_height, thickness = key_thickness, spacing = 0, top_bottom_border = 1.5)
{
    key_count = 16;
    sep_count = 17;
    key_width = (width - sep_count * key_separator_width) / key_count;
    
    color("blue", 1.0)
    linear_extrude (height = thickness)
    for (i=[0:key_count-1])
    {
        translate ([-width/2 + (key_separator_width + key_width + spacing*2) * i + key_separator_width, -height/2])
        {
            square([key_width, height]);
        }
    }
    
    color("green", 1.0)
    linear_extrude (height = thickness)
    for (i=[0:sep_count-1])
    {
        translate ([-width/2 + (key_separator_width + key_width + spacing*2) * i - spacing, -height/2])
        {
            square([key_separator_width, height]);
        }
    }
    color("green", 1.0)
    linear_extrude (height = thickness)
    translate ([-width/2, -height/2 - top_bottom_border - spacing])
    {
        square([width, top_bottom_border]);
    }
    color("green", 1.0)
    linear_extrude (height = thickness)
    translate ([-width/2, height/2 + spacing])
    {
        square([width, top_bottom_border]);
    }
}

module slider_cover(width = top_width, height = slider_height, thickness = top_thickness)
{   
    bevel_x = 26;
    bevel_y = 52;
    y_offset = slider_y_adjust;
    
    extra_hole_size = 3.3;
    extra_hole_x1 = pcb_width_main * 1.5;
    extra_hole_x2 = pcb_width_main * 0.5;
    extra_hole_margin_y = 14;
    
    color ("gray", 0.1)
    linear_extrude (height = thickness)
    difference ()
    {
        translate ([0, y_offset])
        {
            scale ([width, height])
            {
                polygon(points=[[-.5+bevel_x/width,-.5], [.5-bevel_x/width,-.5],
                                [.5,-.5+bevel_y/height], [.5,.5-bevel_y/height],
                                [.5-bevel_x/width,.5], [-.5+bevel_x/width,.5],
                                [-.5,.5-bevel_y/height], [-.5,-.5+bevel_y/height]]);
            }
        }
        slider_holes();
        
        translate ([-extra_hole_x1, y_offset - height/2 + extra_hole_margin_y])
        {
            circle(r = extra_hole_size/2, $fn=32);
        }
        translate ([-extra_hole_x2, y_offset - height/2 + extra_hole_margin_y])
        {
            circle(r = extra_hole_size/2, $fn=32);
        }
        translate ([extra_hole_x2, y_offset - height/2 + extra_hole_margin_y])
        {
            circle(r = extra_hole_size/2, $fn=32);
        }
        translate ([extra_hole_x1, y_offset - height/2 + extra_hole_margin_y])
        {
            circle(r = extra_hole_size/2, $fn=32);
        }
    }
}


module wall(width, height, thickness = wall_thickness, tabs_top = top_thickness, tabs_bottom = bottom_thickness, tab_count = 3)
{   
    tab_width = width / (tab_count*2);
    
    color ("black", 0.4)
    rotate ([90, 0, 0])
    {
        linear_extrude (height = thickness)
        union ()
        {
            translate ([-width/2, -height/2])
            {
                square([width, height]);
            }
            for (i=[0:tab_count-1])
            {
                translate ([-width/2 + tab_width * (i*2+0.5), -height/2 - tabs_top])
                {
                    square([tab_width, tabs_top]);
                }
                translate ([-width/2 + tab_width * (i*2+0.5), height/2])
                {
                    square([tab_width, tabs_bottom]);
                }
            }
        }
    }
}

module box_walls(width = top_width - 53, depth = slider_height - 7, height = wall_height, thickness = wall_thickness, tabs_top = top_thickness, tabs_bottom = bottom_thickness)
{
    union ()
    {
        translate ([0, -depth/2, 0])
        {
            wall(width, height, thickness, tabs_top, tabs_bottom, tab_count = 7);
        }
        translate ([0, depth/2-thickness, 0])
        {
            wall(width, height, thickness, tabs_top, tabs_bottom, tab_count = 7);
        }
        translate ([-width/2, -thickness, 0])
        {
            rotate ([0, 0, 90])
            {
                wall(depth-thickness*2, height, thickness, tabs_top, tabs_bottom);
            }
        }
        translate ([width/2, -thickness, 0])
        {
            rotate ([0, 0, -90])
            {
                wall(depth-thickness*2, height, thickness, tabs_top, tabs_bottom);
            }
        }
    }
}

module box_walls_flat(width = top_width - 53, depth = slider_height - 7, height = wall_height, thickness = wall_thickness, tabs_top = top_thickness, tabs_bottom = bottom_thickness)
{
    // note that the rear panel isn't moved so that the cutouts are easier
    
    height_off = height + tabs_top + tabs_bottom + 1;
    
    union ()
    {
        translate ([0, -depth/2, height_off])
        {
            wall(width, height, thickness, tabs_top, tabs_bottom, tab_count = 7);
        }
        translate ([0, -depth/2, 0])
        {
            wall(width, height, thickness, tabs_top, tabs_bottom, tab_count = 7);
        }
        translate ([0, -depth/2, -height_off])
        {
            wall(depth-thickness*2, height, thickness, tabs_top, tabs_bottom);
        }
        translate ([0, -depth/2, -height_off*2])
        {
            wall(depth-thickness*2, height, thickness, tabs_top, tabs_bottom);
        }
    }
}

module microusb_port(thickness = 26 + wall_thickness)
{
    screw_distance = 28;
    screw_size = 3;
    
    color ("red", 1)
    linear_extrude (height = thickness)
    union ()
    {
        translate ([-screw_distance/2, 0])
        {
            circle(r = screw_size/2, $fn=32);
        }
        translate ([screw_distance/2, 0])
        {
            circle(r = screw_size/2, $fn=32);
        }
        square([10.6, 8.5], center=true);
    }
    
    color ("red", 1)
    translate ([0, 0, -5])
    {
        linear_extrude (height = thickness)
        circle(r = 5/2, $fn=32);
    }
}

module switch_hole(thickness = 6.5 + wall_thickness)
{
    screw_distance = 15;
    screw_size = 2.2;
    
    color ("red", 1)
    linear_extrude (height = thickness)
    union ()
    {
        translate ([-screw_distance/2, 0])
        {
            circle(r = screw_size/2, $fn=32);
        }
        translate ([screw_distance/2, 0])
        {
            circle(r = screw_size/2, $fn=32);
        }
        square([8, 4], center=true);
    }
    
    color ("red", 1)
    translate ([0, 0, -5])
    {
        linear_extrude (height = thickness)
        circle(r = 7/2, $fn=32);
    }
}

module tact_hole(depth = 7, retainer_thickness = 3)
{
    hole_size = 3.8;
    box_size = 6;
    retainer_hole_size = 2.2;
    retainer_hole_gap = (4-retainer_hole_size)/2;
    
    color ("red", 1)
    linear_extrude (height = depth)
    circle(r = hole_size/2, $fn=32);
    
    color ("red", 1)
    linear_extrude (height = 3.5)
    square(box_size, center=true);
    
    color ("red", 1)
    translate ([0, retainer_thickness - 0.1, -retainer_hole_size/2 - retainer_hole_gap])
    {
        rotate ([-90, 0, 0])
        {
            linear_extrude (height = retainer_thickness + 0.1)
            circle(r = retainer_hole_size/2, $fn=32);
        }
    }
}

module slider_3d()
{
    slider_keys();
    translate ([0, 0, key_thickness])
    {
        slider_pcbs();
    }
    difference () // top
    {
        translate ([0, 0, -top_thickness])
        {
            slider_cover();
        }
        translate ([0, slider_y_adjust + wall_thickness, wall_height/2 + 1])
        {
            minkowski()
            {
                box_walls(tabs_top = top_thickness + 2);
                cube(tab_tolerance*2, center = true);
            }
        }
    }
    difference () // bottom
    {
        translate ([0, 0, wall_height])
        {
            color ("black", 0.4)
            slider_cover(bottom_width, thickness = bottom_thickness);
        }
        translate ([0, slider_y_adjust + wall_thickness, wall_height/2 - 1])
        {
            minkowski()
            {
                box_walls(tabs_bottom = bottom_thickness + 2);
                cube(tab_tolerance*2, center = true);
            }
        }
        translate ([-100, -slider_height/2 + slider_y_adjust - wall_thickness + 11, wall_height - 3.2])
        {
            rotate ([90, 0, 0])
            {
                tact_hole();
            }
        }
    }
    difference ()
    {
        translate ([0, slider_y_adjust + wall_thickness, wall_height/2])
        {
            box_walls();
        }
        translate ([0, -slider_height/2 + slider_y_adjust - wall_thickness + 32.5, wall_height - 5.2])
        {
            rotate ([90, 0, 0])
            {
                microusb_port();
            }
        }
        translate ([-75, -slider_height/2 + slider_y_adjust - wall_thickness + 13.5, wall_height - 3.2])
        {
            rotate ([90, 0, 0])
            {
                switch_hole();
            }
        }
        translate ([-100, -slider_height/2 + slider_y_adjust - wall_thickness + 11, wall_height - 3.2])
        {
            rotate ([90, 0, 0])
            {
                tact_hole();
            }
        }
    }
    translate ([0, 0, -top_thickness])
    {
        //linear_extrude (height = full_height)
        //slider_holes();
    }
    translate ([0, -slider_height/2 + slider_y_adjust - wall_thickness + 32.5, wall_height - 5.2])
    {
        rotate ([90, 0, 0])
        {
            //microusb_port();
        }
    }
    translate ([-75, -slider_height/2 + slider_y_adjust - wall_thickness + 13.5, wall_height - 3.2])
    {
        rotate ([90, 0, 0])
        {
            //switch_hole();
        }
    }
    translate ([-100, -slider_height/2 + slider_y_adjust - wall_thickness + 11, wall_height - 3.2])
    {
        rotate ([90, 0, 0])
        {
            //tact_hole();
        }
    }
}

module slider_2d()
{
    slider_keys(spacing = 0.5, top_bottom_border = 2.5);
    translate ([0, 125, top_thickness])
    {
        difference () // top
        {
            translate ([0, 0, -top_thickness])
            {
                slider_cover();
            }
            translate ([0, slider_y_adjust + wall_thickness, wall_height/2 + 1])
            {
                minkowski()
                {
                    box_walls(tabs_top = top_thickness + 2);
                    cube(tab_tolerance*2, center = true);
                }
            }
        }
    }
    translate ([0, 265, -wall_height])
    {
        difference () // bottom
        {
            translate ([0, 0, wall_height])
            {
                color ("black", 0.4)
                slider_cover(bottom_width, thickness = bottom_thickness);
            }
            translate ([0, slider_y_adjust + wall_thickness, wall_height/2 - 1])
            {
                minkowski()
                {
                    box_walls(tabs_bottom = bottom_thickness + 2);
                    cube(tab_tolerance*2, center = true);
                }
            }
            translate ([-100, -slider_height/2 + slider_y_adjust - wall_thickness + 11, wall_height - 3.2])
            {
                rotate ([90, 0, 0])
                {
                    tact_hole();
                }
            }
        }
    }
    translate ([0, -85, slider_height/2 - slider_y_adjust - wall_thickness - 2])
    {
        rotate([90, 0, 0])
        {
            difference ()
            {
                translate ([0, slider_y_adjust + wall_thickness, wall_height/2])
                {
                    box_walls_flat();
                }
                translate ([0, -slider_height/2 + slider_y_adjust - wall_thickness + 32.5, wall_height - 5.2])
                {
                    rotate ([90, 0, 0])
                    {
                        microusb_port();
                    }
                }
                translate ([-75, -slider_height/2 + slider_y_adjust - wall_thickness + 13.5, wall_height - 3.2])
                {
                    rotate ([90, 0, 0])
                    {
                        switch_hole();
                    }
                }
                translate ([-100, -slider_height/2 + slider_y_adjust - wall_thickness + 11, wall_height - 3.2])
                {
                    rotate ([90, 0, 0])
                    {
                        tact_hole();
                    }
                }
            }
        }
    }
}

slider_3d();

projection()
{
    //slider_2d();
}