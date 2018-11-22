namespace eval segment {

  proc segment_tcl { x y f p input_dir } {

    # Call the C function
    set ret [ segment_frame $x $y $f $p $input_dir ]

    # Return a Tcl Dict. Swift will see it as an array.
    return [ \
              dict create \
              0 [ $ret cget -frame_id] \
              1 [ $ret cget -centroid_x ] \
              2 [ $ret cget -centroid_y ] \
              3 [ $ret cget -area] \
           ]

  }
}
