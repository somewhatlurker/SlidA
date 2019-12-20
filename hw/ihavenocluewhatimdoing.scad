
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
                circle(hole_size/2);
            }
            translate ([width_main - hole_dist_x, hole_dist_y, 0])
            {
                circle(hole_size/2);
            }
            translate ([hole_dist_x, height_total - hole_dist_y, 0])
            {
                circle(hole_size/2);
            }
            translate ([width_main - hole_dist_x, height_total - hole_dist_y, 0])
            {
                circle(hole_size/2);
            }
        }
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
    width_total = width_main + led_offset;

    corner_rad = 1.27;
    hole_dist_x = 6.175;
    hole_dist_y = 3.175;
    hole_size = 3.2;
    
    color("white", 1.0)
    difference ()
    {
        for (i=[0:n-1])
        {
            translate ([(-width_main/2) * n + width_main * i, -height_total/2, 0])
            {
                union ()
                {
                    // main body full width (not full height)
                    translate ([0, led_height, 0])
                    {
                        cube([width_main, height_main - corner_rad, thickness]);
                    }
                    // main body full height (not full width)
                    translate ([corner_rad, led_height, 0])
                    {
                        cube([width_main - corner_rad * 2, height_main, thickness]);
                    }
    
                    // main body corners
                    translate ([corner_rad, height_total - corner_rad, 0])
                    {
                        cylinder(h = thickness, r = corner_rad, center = false);
                    }
                    translate ([width_main - corner_rad, height_total - corner_rad, 0])
                    {
                        cylinder(h = thickness, r = corner_rad, center = false);
                    }
                    
                    // led section full width (not full height)
                    translate ([led_x_offset, corner_rad, 0])
                    {
                        cube([width_main, led_height - corner_rad, thickness]);
                    }
                    // led section full height (not full width)
                    translate ([led_x_offset + corner_rad, 0, 0])
                    {
                        cube([width_main - corner_rad * 2, led_height, thickness]);
                    }
                    
                    // led section corners
                    translate ([led_x_offset + corner_rad, corner_rad, 0])
                    {
                        cylinder(h = thickness, r = corner_rad, center = false);
                    }
                    translate ([led_x_offset + width_main - corner_rad, corner_rad, 0])
                    {
                        cylinder(h = thickness, r = corner_rad, center = false);
                    }
                }
            }
        }
        linear_extrude (height = thickness)
        slider_holes(n);
    }
}

module slider_keys (width = 97*4 + 3, height = 81, thickness = 3)
{
    key_count = 16;
    sep_count = 17;
    sep_width = 3;
    key_width = (width - sep_count * sep_width) / key_count;
    top_bottom_border = 1.5;
    
    color("blue", 1.0)
    for (i=[0:key_count-1])
    {
        translate ([-width/2 + (sep_width + key_width) * i + sep_width, -height/2, 0])
        {
            cube([key_width, height, thickness]);
        }
    }
    
    color("green", 1.0)
    for (i=[0:sep_count-1])
    {
        translate ([-width/2 + (sep_width + key_width) * i, -height/2, 0])
        {
            cube([sep_width, height, thickness]);
        }
    }
    color("green", 1.0)
    translate ([-width/2, -height/2 - top_bottom_border, 0])
    {
        cube([width, top_bottom_border, thickness]);
    }
    color("green", 1.0)
    translate ([-width/2, height/2, 0])
    {
        cube([width, top_bottom_border, thickness]);
    }
}

module slider_cover(width = 480, height = 120, thickness = 1.5)
{   
    bevel_x = 0.15;
    bevel_y = 0.7;
    y_offset = -5;
    
    color ("gray", 0.1)
    difference ()
    {
        translate ([0, y_offset , 0])
        {
            scale ([width/2, height/2, 1])
            {
                linear_extrude (height = thickness)
                polygon(points=[[-1+bevel_x,-1], [1-bevel_x,-1], [1,-1+bevel_y], [1,1-bevel_y],
                                [1-bevel_x,1], [-1+bevel_x,1], [-1,1-bevel_y], [-1,-1+bevel_y]]);
            }
        }
        linear_extrude (height = thickness)
        slider_holes();
    }
}

slider_keys();
translate ([0, 0, 3])
{
    slider_pcbs();
}
translate ([0, 0, -1.5])
{
    slider_cover();
}