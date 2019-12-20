
module slider_holes (n = 4)
{
    height_main = 85.7;
    width_main = 97;
    led_height = 8.1;
    
    height_total = height_main + led_height;

    hole_dist_x = 6.175;
    hole_dist_y = 3.175;
    hole_size = 3.2;
    
    for (i=[0:n-1])
    {
        translate ([(-width_main/2) * n + width_main * i, -height_total/2, 0])
        {
            translate ([hole_dist_x, hole_dist_y, 0])
            {
                circle(hole_size/2, $fn=32);
            }
            translate ([width_main - hole_dist_x, hole_dist_y, 0])
            {
                circle(hole_size/2, $fn=32);
            }
            translate ([hole_dist_x, height_total - hole_dist_y, 0])
            {
                circle(hole_size/2, $fn=32);
            }
            translate ([width_main - hole_dist_x, height_total - hole_dist_y, 0])
            {
                circle(hole_size/2, $fn=32);
            }
        }
    }
    
    translate ([(-width_main/2) * n - hole_size/2 - 1.57, 0, 0])
    {
        circle(hole_size/2, $fn=32);
    }
    translate ([(width_main/2) * n + hole_size/2 + 1.57, 0, 0])
    {
        circle(hole_size/2, $fn=32);
    }
}

module slider_pcbs (n = 4)
{
    thickness = 1.6;
    height_main = 85.7;
    width_main = 97;
    
    led_x_offset = 3;
    led_height = 8.1;
    
    height_total = height_main + led_height;
    width_total = width_main + led_x_offset;

    corner_rad = 1.27;
    hole_dist_x = 6.175;
    hole_dist_y = 3.175;
    hole_size = 3.2;
    
    color("white", 1.0)
    linear_extrude (height = thickness)
    difference ()
    {
        for (i=[0:n-1])
        {
            translate ([(-width_main/2) * n + width_main * i, -height_total/2])
            {
                union ()
                {
                    // main body full width (not full height)
                    translate ([0, led_height])
                    {
                        square([width_main, height_main - corner_rad]);
                    }
                    // main body full height (not full width)
                    translate ([corner_rad, led_height])
                    {
                        square([width_main - corner_rad * 2, height_main]);
                    }
    
                    // main body corners
                    translate ([corner_rad, height_total - corner_rad])
                    {
                        circle(r = corner_rad, $fn=32);
                    }
                    translate ([width_main - corner_rad, height_total - corner_rad])
                    {
                        circle(r = corner_rad, $fn=32);
                    }
                    
                    // led section full width (not full height)
                    translate ([led_x_offset, corner_rad])
                    {
                        square([width_main, led_height - corner_rad]);
                    }
                    // led section full height (not full width)
                    translate ([led_x_offset + corner_rad, 0])
                    {
                        square([width_main - corner_rad * 2, led_height]);
                    }
                    
                    // led section corners
                    translate ([led_x_offset + corner_rad, corner_rad])
                    {
                        circle(r = corner_rad, $fn=32);
                    }
                    translate ([led_x_offset + width_main - corner_rad, corner_rad])
                    {
                        circle(r = corner_rad, $fn=32);
                    }
                }
            }
        }
        slider_holes(n);
    }
}

module slider_keys (width = 97*4 + 3, height = 81.1, thickness = 3)
{
    key_count = 16;
    sep_count = 17;
    sep_width = 3;
    key_width = (width - sep_count * sep_width) / key_count;
    top_bottom_border = 1.5;
    
    color("blue", 1.0)
    linear_extrude (height = thickness)
    for (i=[0:key_count-1])
    {
        translate ([-width/2 + (sep_width + key_width) * i + sep_width, -height/2])
        {
            square([key_width, height]);
        }
    }
    
    color("green", 1.0)
    linear_extrude (height = thickness)
    for (i=[0:sep_count-1])
    {
        translate ([-width/2 + (sep_width + key_width) * i, -height/2])
        {
            square([sep_width, height]);
        }
    }
    color("green", 1.0)
    linear_extrude (height = thickness)
    translate ([-width/2, -height/2 - top_bottom_border])
    {
        square([width, top_bottom_border]);
    }
    color("green", 1.0)
    linear_extrude (height = thickness)
    translate ([-width/2, height/2])
    {
        square([width, top_bottom_border]);
    }
}

module slider_cover(width = 500, height = 145, thickness = 1.5)
{   
    bevel_x = 33 / (width/2);
    bevel_y = 48 / (height/2);
    y_offset = -5;
    
    color ("gray", 0.1)
    linear_extrude (height = thickness)
    difference ()
    {
        translate ([0, y_offset])
        {
            scale ([width/2, height/2])
            {
                polygon(points=[[-1+bevel_x,-1], [1-bevel_x,-1], [1,-1+bevel_y], [1,1-bevel_y],
                                [1-bevel_x,1], [-1+bevel_x,1], [-1,1-bevel_y], [-1,-1+bevel_y]]);
            }
        }
        slider_holes();
    }
}


module wall(width, height, thickness = 2, tabs_top = 1.5, tabs_bottom = 2.0)
{   
    tab_count = 3;
    tab_width = width / (tab_count*2+1);
    
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
                translate ([-width/2 + width / (tab_count*2+1) * (i*2+1), -height/2 - tabs_top])
                {
                    square([tab_width, tabs_top]);
                }
                translate ([-width/2 + width / (tab_count*2+1) * (i*2+1), height/2])
                {
                    square([tab_width, tabs_bottom]);
                }
            }
        }
    }
}

module box_walls(width = 500-72, depth = 138, height = 12.6, thickness = 2, tabs_top = 1.5, tabs_bottom = 2.0)
{
    union ()
    {
        translate ([0, -depth/2, 0])
        {
            wall(width, height, thickness, tabs_top, tabs_bottom);
        }
        translate ([0, depth/2-thickness, 0])
        {
            wall(width, height, thickness, tabs_top, tabs_bottom);
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

module microusb_port(thickness = 27)
{
    screw_distance = 28;
    screw_size = 2.3;
    
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

slider_keys();
translate ([0, 0, 3])
{
    slider_pcbs();
}
difference () // top
{
    translate ([0, 0, -1.5])
    {
        slider_cover();
    }
    translate ([0, -3, 12.6/2 + 1])
    {
        box_walls(tabs_top=4);
    }
}
difference () // bottom
{
    translate ([0, 0, 12.6])
    {
        color ("black", 0.4)
        slider_cover(560, thickness = 2);
    }
    translate ([0, -3, 12.6/2 - 1])
    {
        box_walls(tabs_bottom=4);
    }
}
difference ()
{
    translate ([0, -3, 12.6/2])
    {
        box_walls();
    }
    translate ([0, -48, 7.4])
    {
        rotate ([90, 0, 0])
        {
            microusb_port();
        }
    }
}
translate ([0, 0, -1.5])
{
    //linear_extrude (height = 16.1)
    //slider_holes();
}
translate ([0, -48, 7.4])
{
    rotate ([90, 0, 0])
    {
        //microusb_port();
    }
}