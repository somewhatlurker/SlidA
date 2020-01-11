pcb_width_main = 97;
pcb_width_ledsoffset = 3;
pcb_width_total = pcb_width_main + pcb_width_ledsoffset;
pcb_height_main = 85.7;
pcb_height_leds = 8.1;
pcb_height_total = pcb_height_main + pcb_height_leds;

pcb_hole_dist_x = 6.175;
pcb_hole_dist_y = 3.175;
pcb_hole_size = 3.1;

top_thickness = 2;
key_thickness = 3;
pcb_thickness = 1.6;
space_thickness = 8;
bottom_thickness = 2;
wall_thickness = 2;

wall_height = key_thickness + pcb_thickness + space_thickness;
full_height = top_thickness + wall_height + bottom_thickness;

slider_height = 136;
top_width = 500;
bottom_width = 500;
slider_y_adjust = -9.5;

slider_bevel_x = 44;
slider_bevel_y = 28;

key_separator_width = 3;
key_top_bottom_padding = top_thickness;
key_area_width = pcb_width_main * 4 + key_separator_width;
key_area_height = pcb_height_total - pcb_hole_dist_y*2 - pcb_hole_size - key_top_bottom_padding*2 - .25;

tab_tolerance = 0.05;

hole_resolution = 50;

corner_rounding = 2;


module slider_holes (n = 4)
{
    for (i=[0:n-1])
    {
        translate ([(-pcb_width_main/2) * n + pcb_width_main * i, -pcb_height_total/2, 0])
        {
            translate ([pcb_hole_dist_x, pcb_hole_dist_y, 0])
            {
                circle(pcb_hole_size/2, $fn=hole_resolution);
            }
            translate ([pcb_width_main - pcb_hole_dist_x, pcb_hole_dist_y, 0])
            {
                circle(pcb_hole_size/2, $fn=hole_resolution);
            }
            translate ([pcb_hole_dist_x, pcb_height_total - pcb_hole_dist_y, 0])
            {
                circle(pcb_hole_size/2, $fn=hole_resolution);
            }
            translate ([pcb_width_main - pcb_hole_dist_x, pcb_height_total - pcb_hole_dist_y, 0])
            {
                circle(pcb_hole_size/2, $fn=hole_resolution);
            }
        }
    }
    
    translate ([-key_area_width/2 - pcb_hole_size/2 - .15, 0, 0])
    {
        circle(pcb_hole_size/2, $fn=hole_resolution);
    }
    translate ([key_area_width/2 + pcb_hole_size/2 + .15, 0, 0])
    {
        circle(pcb_hole_size/2, $fn=hole_resolution);
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
                        circle(r = corner_rad, $fn=hole_resolution);
                    }
                    translate ([pcb_width_main - corner_rad, pcb_height_total - corner_rad])
                    {
                        circle(r = corner_rad, $fn=hole_resolution);
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
                        circle(r = corner_rad, $fn=hole_resolution);
                    }
                    translate ([pcb_width_ledsoffset + pcb_width_main - corner_rad, corner_rad])
                    {
                        circle(r = corner_rad, $fn=hole_resolution);
                    }
                }
            }
        }
        slider_holes(n);
    }
}

module slider_keys (width = key_area_width, height = key_area_height, thickness = key_thickness, top_gap = 0, top_border = key_top_bottom_padding, bottom_border = key_top_bottom_padding + pcb_hole_size + 1.5, top_border_retainer = key_top_bottom_padding)
{
    key_count = 16;
    sep_width = key_separator_width;
    key_width = ((width - (key_count + 1) * key_separator_width) / key_count);
    key_height = height;
    
    
    // keys
    color("blue", 1.0)
    linear_extrude (height = thickness)
    for (i=[0:key_count-1])
    {
        translate ([-width/2 + (sep_width + key_width) * i + sep_width + 0.075, -key_height/2])
        {
            square([key_width - 0.15, key_height - 0.1]);
        }
    }
    
    // separators + frame
    color("green", 1.0)
    linear_extrude (height = thickness)
    difference ()
    {
        union ()
        {
            // separators + frame
            for (i=[0:(key_count)/4 - 1])
            {
                for (j=[0:3])
                {
                    translate ([-width/2 + (sep_width + key_width) * (i*4 + j), -key_height/2])
                    {
                        square([sep_width, key_height]);
                    }
                }
                
                // bottom strip
                translate ([-width/2 + (sep_width + key_width) * i*4, key_height/2])
                {
                    difference ()
                    {
                        square([sep_width*4.5 + key_width*4 - pcb_hole_dist_x + pcb_hole_size/2 + 2, bottom_border]);
                        
                        // cutouts to help control light leakage
                        for (j=[0:3])
                        {
                            translate ([-0.3 + (sep_width + key_width) * j, 0])
                            {
                                square([0.3, bottom_border - 3]);
                            }
                            translate ([sep_width + (sep_width + key_width) * j, 0])
                            {
                                square([0.3, bottom_border - 3]);
                            }
                        }
                    }
                }
            }
            // messy hack to add last separator
            translate ([width/2 - sep_width, -key_height/2])
            {
                square([sep_width, key_height]);
            }
            translate ([width/2 - sep_width/2 - pcb_hole_dist_x + pcb_hole_size/2, key_height/2])
            {
                difference ()
                {
                    square([sep_width/2 + pcb_hole_dist_x - pcb_hole_size/2, bottom_border]);
                    
                    translate ([-0.3 + pcb_hole_dist_x - pcb_hole_size/2 - sep_width/2, 0])
                    {
                        square([0.3, bottom_border - 3]);
                    }
                }
            }
            
            // messy hack for top retainer's retention
            translate ([-width/2, -key_height/2 - top_border_retainer])
            {
                square([sep_width, bottom_border]);
            }
            translate ([width/2 - sep_width, -key_height/2 - top_border_retainer])
            {
                square([sep_width, bottom_border]);
            }
        }
        
        slider_holes(4);
    }
    
    
    // top retainer
    color("red", 1.0)
    linear_extrude (height = thickness)
    {
        translate ([-width/2 + sep_width + 0.1, -key_height/2 - top_border])
        {
            square([(key_width + sep_width) * 4 - 0.1, top_border]);
        }
        translate ([-width/2 + sep_width + (key_width + sep_width) * 4 + 0.1, -key_height/2 - top_border])
        {
            square([(key_width + sep_width) * 4 - 0.1, top_border]);
        }
        translate ([-width/2 + sep_width + (key_width + sep_width) * 8 + 0.1, -key_height/2 - top_border])
        {
            square([(key_width + sep_width) * 4 - 0.1, top_border]);
        }
        translate ([-width/2 + sep_width + (key_width + sep_width) * 12 + 0.1, -key_height/2 - top_border])
        {
            square([(key_width + sep_width) * 4 - sep_width - 0.2, top_border]);
        }
    }
}

module slider_cover(width = top_width, height = slider_height, thickness = top_thickness)
{   
    bevel_x = slider_bevel_x;
    bevel_y = slider_bevel_y;
    y_offset = slider_y_adjust;
    
    extra_hole_size = pcb_hole_size + 0.1;
    //extra_hole_x1 = pcb_width_main * 1.5;
    //extra_hole_x2 = pcb_width_main * 0.5;
    extra_hole_x1 = pcb_width_main * 1;
    extra_hole_x2 = pcb_width_main * 1;
    extra_hole_margin_y = 14;
    
    color ("gray", 0.1)
    linear_extrude (height = thickness)
    difference ()
    {
        translate ([0, y_offset])
        {
            offset(r=corner_rounding, $fn=hole_resolution)
            offset(delta=-corner_rounding, $fn=hole_resolution)
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
            circle(r = extra_hole_size/2, $fn=hole_resolution);
        }
        translate ([-extra_hole_x2, y_offset - height/2 + extra_hole_margin_y])
        {
            circle(r = extra_hole_size/2, $fn=hole_resolution);
        }
        translate ([extra_hole_x2, y_offset - height/2 + extra_hole_margin_y])
        {
            circle(r = extra_hole_size/2, $fn=hole_resolution);
        }
        translate ([extra_hole_x1, y_offset - height/2 + extra_hole_margin_y])
        {
            circle(r = extra_hole_size/2, $fn=hole_resolution);
        }
    }
}


module wall(width, height, thickness = wall_thickness, tabs_top = top_thickness, tabs_bottom = bottom_thickness, tab_count = 3, expand = 0)
{   
    tab_width = width / (tab_count*2);
    
    color ("black", 0.4)
    rotate ([90, 0, 0])
    {
        translate([0,0,-expand])
        linear_extrude (height = thickness + expand*2)
        offset(delta=expand)
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

module box_walls(width = top_width - slider_bevel_x*2, depth = slider_height - 7, height = wall_height, thickness = wall_thickness, tabs_top = top_thickness, tabs_bottom = bottom_thickness, expand = 0)
{
    union ()
    {
        translate ([0, -depth/2, 0])
        {
            wall(width, height, thickness, tabs_top, tabs_bottom, tab_count = 7, expand = expand);
        }
        translate ([0, depth/2-thickness, 0])
        {
            wall(width, height, thickness, tabs_top, tabs_bottom, tab_count = 7, expand = expand);
        }
        translate ([-width/2, -thickness, 0])
        {
            rotate ([0, 0, 90])
            {
                wall(depth-thickness*2, height, thickness, tabs_top, tabs_bottom, expand = expand);
            }
        }
        translate ([width/2, -thickness, 0])
        {
            rotate ([0, 0, -90])
            {
                wall(depth-thickness*2, height, thickness, tabs_top, tabs_bottom, expand = expand);
            }
        }
    }
}

module box_walls_flat(width = top_width - slider_bevel_x*2, depth = slider_height - 7, height = wall_height, thickness = wall_thickness, tabs_top = top_thickness, tabs_bottom = bottom_thickness, expand = 0)
{
    // note that the rear panel isn't moved so that the cutouts are easier
    
    height_off = height + tabs_top + tabs_bottom + 1;
    
    union ()
    {
        translate ([0, -depth/2, height_off])
        {
            wall(width, height, thickness, tabs_top, tabs_bottom, tab_count = 7, expand = expand);
        }
        translate ([0, -depth/2, 0])
        {
            wall(width, height, thickness, tabs_top, tabs_bottom, tab_count = 7, expand = expand);
        }
        translate ([-width/2 + (depth-thickness*2)/2, -depth/2, -height_off])
        {
            wall(depth-thickness*2, height, thickness, tabs_top, tabs_bottom, expand = expand);
        }
        translate ([-width/2 + (depth-thickness*2) * 1.5 + 1, -depth/2, -height_off])
        {
            wall(depth-thickness*2, height, thickness, tabs_top, tabs_bottom, expand = expand);
        }
    }
}

module air_arm_shape(width = 25, top_length = top_width/2 - slider_bevel_x, bottom_length = sqrt(pow(slider_bevel_x,2) + pow(slider_bevel_y,2)), top_angle = atan(slider_bevel_y/slider_bevel_x), base_height = full_height + 0.2, base_length = (slider_height - slider_bevel_y*2) * 0.8, screw_hole_size = 3.2, expand = 0)
{
    sensor_hole_spacing = top_length*0.7 / 5.75;
    
    tab_size = 5.5;
    
    offset(delta=expand)
    union()
    {
        difference()
        {
            offset(r=1, $fn=hole_resolution)
            offset(delta=-1, $fn=hole_resolution)
            offset(r=corner_rounding, $fn=hole_resolution)
            offset(delta=-corner_rounding, $fn=hole_resolution)
            union ()
            {
                rotate([0, 0, top_angle])
                translate([0, -top_length])
                {
                    difference()
                    {
                        square([width, top_length]);
                        
                        translate([width/2, sensor_hole_spacing*0.35])
                        circle(screw_hole_size/2, $fn=hole_resolution);
                        
                        translate([width/4, sensor_hole_spacing*7.0])
                        circle(screw_hole_size/2, $fn=hole_resolution);
                    }
                }
                
                difference()
                {
                    square([width, bottom_length]);
                    
                    translate([width*3/4, bottom_length/2])
                    circle(screw_hole_size/2, $fn=hole_resolution);
                }
                
                translate([width, bottom_length])
                {
                    rotate([0, 0, 10])
                    translate([-width, 0])
                    square([base_length, base_height]);
                }
            }
            
            translate([width, bottom_length])
            {
                rotate([0, 0, 10])
                translate([-width, -0.1])
                {
                    translate([tab_size, 0])
                    {
                        //square([width - tab_size*2, top_thickness + 0.4]);
                        translate([0, base_height - bottom_thickness - 0.2])
                        square([width - tab_size*2, bottom_thickness + 0.4]);
                    }
                    
                    translate([width, 0])
                    {
                        square([base_length - width * 2, top_thickness + 0.4]);
                        
                        translate([0, base_height - bottom_thickness - 0.2])
                        square([base_length - width * 2, bottom_thickness + 0.4]);
                    }
                    
                    // ARM BASE HOLES
                    translate([width*1.555, base_height/2])
                    {
                        circle(screw_hole_size/2, $fn=hole_resolution);
                    }
                    
                    translate([width*1.555 + width*1.06 - tab_size, base_height/2])
                    {
                        circle(screw_hole_size/2, $fn=hole_resolution);
                    }
                    
                    translate([base_length - width + tab_size, 0])
                    {
                        square([width - tab_size*2, top_thickness + 0.4]);
                        
                        translate([0, base_height - bottom_thickness - 0.2])
                        square([width - tab_size*2, bottom_thickness + 0.4]);
                    }
                }
            }
        }
        
        translate([width, bottom_length])
        {
            rotate([0, 0, 10])
            {
                translate([-width, 0])
                {
                    square([tab_size, base_height]);
                }
                
                translate([base_length - tab_size - width, 0])
                {
                    square([tab_size, top_thickness + 2]);
                }
                
                translate([base_length - tab_size - width, base_height - bottom_thickness - 1])
                {
                    square([tab_size, bottom_thickness + 1]);
                }
            }
        }
    }
}

module air_arm_front(width = 25, top_length = top_width/2 - slider_bevel_x, bottom_length = sqrt(pow(slider_bevel_x,2) + pow(slider_bevel_y,2)), top_angle = atan(slider_bevel_y/slider_bevel_x), base_height = full_height + 0.2, base_length = (slider_height - slider_bevel_y*2) * 0.8, thickness = wall_thickness, expand = 0)
{
    sensor_hole_spacing = top_length*0.7 / 5.75;
    sensor_hole_size = 3.1;
    
    color("red", 1.0)
    translate([0,0,-expand])
    linear_extrude (height = thickness + expand*2)
    difference()
    {
        air_arm_shape(expand = expand);
        
        rotate([0, 0, top_angle])
        translate([0, -top_length])
        {
            for (i=[0:6-1])
            {
                translate([width/2, sensor_hole_spacing*(i+0.75)])
                circle(sensor_hole_size/2, $fn=hole_resolution);
            }
        }
        
        translate([width, bottom_length])
        {
            rotate([0, 0, 10])
            {
                translate([width*0.505 - 5.5/2 - 5, 0])
                square([5, top_thickness + 7]);
            }
        }
    }
}

module air_leds(width = 25, top_length = top_width/2 - slider_bevel_x, top_angle = atan(slider_bevel_y/slider_bevel_x), thickness = wall_thickness)
{
    sensor_hole_spacing = top_length*0.7 / 5.75;
    sensor_led_size = 3;
    sensor_led_height = 5.4;
    
    color("black", 1.0)
    translate([0,0,-(sensor_led_height - thickness) + 1])
    union()
    {        
    rotate([0, 0, top_angle])
    translate([0, -top_length])
    {
        for (i=[0:6-1])
        {
            translate([width/2, sensor_hole_spacing*(i+0.75), sensor_led_size/2])
            {
                sphere(sensor_led_size/2, $fn=hole_resolution);
                
                linear_extrude (height = sensor_led_height - sensor_led_size/2)
                circle(sensor_led_size/2, $fn=hole_resolution);
                
                translate([0, 0, sensor_led_height - sensor_led_size/2 - 1])
                linear_extrude (height = 1)
                circle((sensor_led_size + 0.8)/2, $fn=hole_resolution);
            }
        }
    }
    }
}

module air_arm_inner(width = 25, top_length = top_width/2 - slider_bevel_x, bottom_length = sqrt(pow(slider_bevel_x,2) + pow(slider_bevel_y,2)), top_angle = atan(slider_bevel_y/slider_bevel_x), base_height = full_height + 0.2, base_length = (slider_height - slider_bevel_y*2) * 0.8, thickness = wall_thickness, expand = 0)
{
    sensor_hole_spacing = top_length*0.7 / 5.75;
    
    color("red", 1.0)
    translate([0,0,-expand])
    linear_extrude (height = thickness + expand*2)
    union()
    {
        difference()
        {
            air_arm_shape(screw_hole_size = 5.5, expand = expand);
            
            offset(delta = -5)
            air_arm_shape(screw_hole_size = 1, expand = expand);
            
            translate([width, bottom_length])
            {
                rotate([0, 0, 10])
                {
                    translate([-14, base_height/2 - 6.5/2 + (top_thickness - bottom_thickness)/2])
                    square([14 + width*0.505 - 5.5/2, 6.5]);
                    
                    translate([width*0.505 - 5.5/2 - 5, 0])
                    square([5, top_thickness + 7]);
                }
            }
        }
        
        rotate([0, 0, top_angle])
        translate([0, -top_length])
        {
            translate([0, sensor_hole_spacing*0.35 - 5.5/2])
            square([width/2 - 5.5/2, 5.5]);
            
            translate([width/2 + 5.5/2, sensor_hole_spacing*0.35 - 5.5/2])
            square([width/2 - 5.5/2, 5.5]);
            
            translate([2, 0])
            square([width - 4, sensor_hole_spacing*0.35 - 5.5/2]);
            
            
            translate([0, sensor_hole_spacing*7.0 - 5.5/2])
            square([width/4 - 5.5/2, 5.5]);
        }
        
        translate([width*3/4 + 5.5/2, bottom_length/2 - 5.5/2])
        square([width/4 - 5.5/2, 5.5]);
    }
}

module air_arm_back(width = 25, top_length = top_width/2 - slider_bevel_x, top_angle = atan(slider_bevel_y/slider_bevel_x), thickness = wall_thickness, expand = 0)
{
    sensor_hole_spacing = top_length*0.7 / 5.75;
    
    color("red", 1.0)
    translate([0,0,-expand])
    linear_extrude (height = thickness + expand*2)
    union() {
        air_arm_shape(expand = expand);
        
        offset(delta=expand)
        offset(r=1.2, $fn=hole_resolution)
        offset(delta=-1.2, $fn=hole_resolution)
        rotate([0, 0, top_angle])
        translate([width, -top_length + sensor_hole_spacing * 1.25 - 26/2])
        {
            polygon([[-2,1], [7.5, 0], [7.5, 26], [-2, 25]]);
        }
    }
}

module air_arm_full(thickness = wall_thickness, expand = 0)
{
    air_arm_front(thickness = thickness + 0.2, expand = expand);
    air_leds(thickness = thickness);
    
    translate([0, 0, thickness])
    air_arm_inner(thickness = thickness * 4 + 0.2, expand = expand);
    
    translate([0, 0, thickness*5])
    air_arm_back(thickness = thickness, expand = expand);
}

module air_arm_full_2d(thickness = wall_thickness, expand = 0)
{
    offset_x = 38;
    offset_y = -11;
    
    air_arm_front(thickness = thickness, expand = expand);
    
    translate([offset_x, offset_y])
    air_arm_inner(thickness = thickness, expand = expand);
    
    translate([offset_x*2, offset_y*2])
    air_arm_inner(thickness = thickness, expand = expand);
    
    translate([offset_x*3, offset_y*3])
    air_arm_inner(thickness = thickness, expand = expand);
    
    translate([offset_x*4, offset_y*4])
    air_arm_inner(thickness = thickness, expand = expand);
    
    translate([offset_x*5, offset_y*5])
    air_arm_back(thickness = thickness, expand = expand);
}

module microusb_port(thickness = 26 + wall_thickness)
{
    screw_distance = 28;
    screw_size = 3.1;
    
    color ("red", 1)
    linear_extrude (height = thickness)
    union ()
    {
        translate ([-screw_distance/2, 0])
        {
            circle(r = screw_size/2, $fn=hole_resolution);
        }
        translate ([screw_distance/2, 0])
        {
            circle(r = screw_size/2, $fn=hole_resolution);
        }
        square([10.6, 8.5], center=true);
    }
    
    color ("red", 1)
    translate ([0, 0, -5])
    {
        linear_extrude (height = thickness)
        circle(r = 5/2, $fn=hole_resolution);
    }
}

module switch_hole(thickness = 6.5 + wall_thickness)
{
    screw_distance = 15;
    screw_size = 2.1;
    
    color ("red", 1)
    linear_extrude (height = thickness)
    union ()
    {
        translate ([-screw_distance/2, 0])
        {
            circle(r = screw_size/2, $fn=hole_resolution);
        }
        translate ([screw_distance/2, 0])
        {
            circle(r = screw_size/2, $fn=hole_resolution);
        }
        square([8, 4], center=true);
    }
    
    color ("red", 1)
    translate ([0, 0, -5])
    {
        linear_extrude (height = thickness)
        circle(r = 7/2, $fn=hole_resolution);
    }
}

module tact_hole(depth = 7, retainer_thickness = 3)
{
    hole_size = 3.8;
    box_size = 6;
    retainer_hole_size = 2.1;
    retainer_hole_gap = (4-retainer_hole_size-0.1)/2;
    
    color ("red", 1)
    linear_extrude (height = depth)
    circle(r = hole_size/2, $fn=hole_resolution);
    
    color ("red", 1)
    linear_extrude (height = 3.5)
    square(box_size, center=true);
    
    color ("red", 1)
    translate ([0, retainer_thickness - 0.1, -retainer_hole_size/2 - retainer_hole_gap])
    {
        rotate ([-90, 0, 0])
        {
            linear_extrude (height = retainer_thickness + 0.1)
            circle(r = retainer_hole_size/2, $fn=hole_resolution);
        }
    }
}

module rubber_foot(size = 10, thickness = 3)
{
    color ("black", 1)
    cylinder(thickness, size/2, size/2 * 0.8);
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
        translate ([0, slider_y_adjust + wall_thickness, 1])
        {
            box_walls(height = 0, tabs_top = top_thickness + 2, tabs_bottom = 0, expand = tab_tolerance);
        }
        
        union()
        {
            translate ([top_width/2 -13, (slider_height - slider_bevel_y*2) * -0.6 - 1, sqrt(pow(slider_bevel_x,2) + pow(slider_bevel_y,2)) * -0.98 + 4 - top_thickness])
            rotate([90, 10, 90])
            {
                air_arm_back(thickness = 13.1, expand = tab_tolerance*4);
            }
            translate ([-top_width/2 - 0.1, (slider_height - slider_bevel_y*2) * -0.6 - 1, sqrt(pow(slider_bevel_x,2) + pow(slider_bevel_y,2)) * -0.98 + 4 - top_thickness])
            rotate([90, 10, 90])
            {
                air_arm_back(thickness = 13.1, expand = tab_tolerance*4);
            }
        }
    }
    difference () // bottom
    {
        translate ([0, 0, wall_height])
        {
            color ("black", 0.4)
            slider_cover(bottom_width, thickness = bottom_thickness);
            
            translate ([-bottom_width/2 + 44, -slider_height/2 + slider_y_adjust + 24, bottom_thickness])
            {
                rubber_foot();
            }
            translate ([bottom_width/2 - 44, -slider_height/2 + slider_y_adjust + 24, bottom_thickness])
            {
                rubber_foot();
            }
            translate ([-bottom_width/2 + 44, slider_height/2 + slider_y_adjust - 24, bottom_thickness])
            {
                rubber_foot();
            }
            translate ([bottom_width/2 - 44, slider_height/2 + slider_y_adjust - 24, bottom_thickness])
            {
                rubber_foot();
            }
        }
        translate ([0, slider_y_adjust + wall_thickness, wall_height - 1])
        {
            box_walls(height = 0, tabs_top = 0, tabs_bottom = bottom_thickness + 2, expand = tab_tolerance);
        }
        translate ([-105, -slider_height/2 + slider_y_adjust - wall_thickness + 11, wall_height - 3.2])
        {
            rotate ([90, 0, 0])
            {
                tact_hole();
            }
        }
        
        union()
        {
            translate ([top_width/2 -13, (slider_height - slider_bevel_y*2) * -0.6 - 1, sqrt(pow(slider_bevel_x,2) + pow(slider_bevel_y,2)) * -0.98 + 4 - top_thickness])
            rotate([90, 10, 90])
            {
                air_arm_back(thickness = 13.1, expand = tab_tolerance*4);
            }
            translate ([-top_width/2 - 0.1, (slider_height - slider_bevel_y*2) * -0.6 - 1, sqrt(pow(slider_bevel_x,2) + pow(slider_bevel_y,2)) * -0.98 + 4 - top_thickness])
            rotate([90, 10, 90])
            {
                air_arm_back(thickness = 13.1, expand = tab_tolerance*4);
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
        translate ([-80, -slider_height/2 + slider_y_adjust - wall_thickness + 13.5, wall_height - 3.2])
        {
            rotate ([90, 0, 0])
            {
                switch_hole();
            }
        }
        translate ([-105, -slider_height/2 + slider_y_adjust - wall_thickness + 11, wall_height - 3.2])
        {
            rotate ([90, 0, 0])
            {
                tact_hole();
            }
        }
        
        union()
        {
            translate ([top_width/2 - slider_bevel_x + 9, -slider_height/2 - 35, wall_thickness + tab_tolerance*3 + 0.25])
            rotate([180, 0, atan(slider_bevel_y/slider_bevel_x) + 90])
            air_arm_back(expand = tab_tolerance*0, thickness = wall_thickness + tab_tolerance*3);
            
            translate ([-top_width/2 + slider_bevel_x - 9, -slider_height/2 - 35, wall_thickness + tab_tolerance*3 + 0.25])
            scale([-1, 1, 1])
            rotate([180, 0, atan(slider_bevel_y/slider_bevel_x) + 90])
            air_arm_back(expand = tab_tolerance*0, thickness = wall_thickness + tab_tolerance*3);
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
            microusb_port();
        }
    }
    translate ([-80, -slider_height/2 + slider_y_adjust - wall_thickness + 13.5, wall_height - 3.2])
    {
        rotate ([90, 0, 0])
        {
            switch_hole();
        }
    }
    translate ([-105, -slider_height/2 + slider_y_adjust - wall_thickness + 11, wall_height - 3.2])
    {
        rotate ([90, 0, 0])
        {
            tact_hole();
        }
    }
    
    translate ([-top_width/2 + slider_bevel_x - 9, -slider_height/2 - 35, wall_height])
    scale([-1, 1, 1])
    rotate([180, 0, atan(slider_bevel_y/slider_bevel_x) + 90])
    air_arm_full();

    translate ([top_width/2 - 13, (slider_height - slider_bevel_y*2) * -0.6 - 1, sqrt(pow(slider_bevel_x,2) + pow(slider_bevel_y,2)) * -0.98 + 4 - top_thickness])
    rotate([90, 10, 90])
    air_arm_full();
}

module slider_2d_keys()
{
    slider_keys (top_gap = 0.4, top_border = 0);
}

module slider_2d_top()
{
    translate ([0, 0, top_thickness])
    {
        difference () // top
        {
            translate ([0, 0, -top_thickness])
            {
                slider_cover();
            }
            translate ([0, slider_y_adjust + wall_thickness, 1])
            {
                box_walls(height = 0, tabs_top = top_thickness + 2, tabs_bottom = 0, expand = tab_tolerance);
            }
            
            union()
            {
                translate ([top_width/2 -13, (slider_height - slider_bevel_y*2) * -0.6 - 1, sqrt(pow(slider_bevel_x,2) + pow(slider_bevel_y,2)) * -0.98 + 4 - top_thickness])
                rotate([90, 10, 90])
                {
                    air_arm_back(thickness = 13.1, expand = tab_tolerance*4);
                }
                translate ([-top_width/2 - 0.1, (slider_height - slider_bevel_y*2) * -0.6 - 1, sqrt(pow(slider_bevel_x,2) + pow(slider_bevel_y,2)) * -0.98 + 4 - top_thickness])
                rotate([90, 10, 90])
                {
                    air_arm_back(thickness = 13.1, expand = tab_tolerance*4);
                }
            }
        }
    }
    
    translate ([0, - slider_height/2 + slider_y_adjust - 2, 0])
    {
        slider_keys (height = 0, top_gap = 0.4, top_border = key_thickness - 0.3, bottom_border = 0);
    }
    
    translate ([0, - slider_height/2 + slider_y_adjust - 2 - key_thickness, 0])
    {
        slider_keys (height = 0, top_gap = 0.4, top_border = key_thickness - 0.3, bottom_border = 0);
    }
}

module slider_2d_bottom()
{
    translate ([0, 0, -wall_height])
    {
        difference () // bottom
        {
            translate ([0, 0, wall_height])
            {
                color ("black", 0.4)
                slider_cover(bottom_width, thickness = bottom_thickness);
            }
            translate ([0, slider_y_adjust + wall_thickness, wall_height - 1])
            {
                box_walls(height = 0, tabs_top = 0, tabs_bottom = bottom_thickness + 2, expand = tab_tolerance);
            }
            translate ([-105, -slider_height/2 + slider_y_adjust - wall_thickness + 11, wall_height - 3.2])
            {
                rotate ([90, 0, 0])
                {
                    tact_hole();
                }
            }
            
            union()
            {
                translate ([top_width/2 -13, (slider_height - slider_bevel_y*2) * -0.6 - 1, sqrt(pow(slider_bevel_x,2) + pow(slider_bevel_y,2)) * -0.98 + 4 - top_thickness])
                rotate([90, 10, 90])
                {
                    air_arm_back(thickness = 13.1, expand = tab_tolerance*4);
                }
                translate ([-top_width/2 - 0.1, (slider_height - slider_bevel_y*2) * -0.6 - 1, sqrt(pow(slider_bevel_x,2) + pow(slider_bevel_y,2)) * -0.98 + 4 - top_thickness])
                rotate([90, 10, 90])
                {
                    air_arm_back(thickness = 13.1, expand = tab_tolerance*4);
                }
            }
        }
    }
}

module slider_2d_walls()
{
    translate ([0, 0, slider_height/2 - slider_y_adjust - wall_thickness - 2])
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
                translate ([-80, -slider_height/2 + slider_y_adjust - wall_thickness + 13.5, wall_height - 3.2])
                {
                    rotate ([90, 0, 0])
                    {
                        switch_hole();
                    }
                }
                translate ([-105, -slider_height/2 + slider_y_adjust - wall_thickness + 11, wall_height - 3.2])
                {
                    rotate ([90, 0, 0])
                    {
                        tact_hole();
                    }
                }
                union()
                {
                    translate ([top_width/2 - slider_bevel_x + 9, -slider_height/2 - 35, wall_thickness + tab_tolerance*3 + 0.25])
                    rotate([180, 0, atan(slider_bevel_y/slider_bevel_x) + 90])
                    air_arm_back(expand = tab_tolerance*0, thickness = wall_thickness + tab_tolerance*3);
                    
                    translate ([-top_width/2 + slider_bevel_x - 9, -slider_height/2 - 35, wall_thickness + tab_tolerance*3 + 0.25])
                    scale([-1, 1, 1])
                    rotate([180, 0, atan(slider_bevel_y/slider_bevel_x) + 90])
                    air_arm_back(expand = tab_tolerance*0, thickness = wall_thickness + tab_tolerance*3);
                }
            }
        }
    }
}

module slider_2d_air()
{
    rotate ([0, 0, atan(slider_bevel_y/slider_bevel_x)/2])
    {
        translate ([-20*6 - top_width*0.12 - slider_height*2/3, 0, 0])
        {
            air_arm_full_2d();
        }
        translate ([10*6 + top_width*0.12 + slider_height*2/3, 0, 0])
        {
            translate([0, 25*6, 0])
            rotate([0, 0, atan(slider_bevel_y/slider_bevel_x)*2 - 30])
            scale([-1, 1, 1])
            air_arm_full_2d();
        }
    }
}

module slider_2d_full()
{
    translate ([0, - key_thickness*2 - 2, 0])
    {
        slider_2d_keys();
    }
    
    translate ([0, key_area_height/2 + slider_height/2 - slider_y_adjust + pcb_hole_size + 4, 0])
    {
        slider_2d_top();
    }
    
    translate ([0, key_area_height/2 + slider_height/2 - slider_y_adjust + key_thickness + slider_height + pcb_hole_size + 4, 0])
    {
        slider_2d_bottom();
    }
    
    translate ([0, -key_area_height/2 - full_height*1 - key_thickness*2 - 7, 0])
    {
        slider_2d_walls();
    }
    
    translate ([0, slider_height*2 + 25 + top_width/2, 0])
    {
        slider_2d_air();
    }
}

slider_3d();
//slider_2d_full();

projection()
{
    scale([1, -1, 1])
    {
        //slider_2d_keys();
        //slider_2d_top();
        //slider_2d_bottom();
        //slider_2d_walls();
        //slider_2d_air();
    }
}

//translate ([top_width/2 + 13, (slider_height - slider_bevel_y*2) * -0.6 + 50, sqrt(pow(slider_bevel_x,2) + pow(slider_bevel_y,2)) * -0.98 + 4 - top_thickness])
//rotate([90, 10, 90])
//air_arm_full_2d();

//translate ([230, -104, -300])
//cube([10, 50, 300]);
