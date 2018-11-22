import files;
import io;
import string;
import sys;
@dispatch=WORKER

(int ret[]) segment(int x, int y, int frame, int padding, string input_dir) "segment" "0.0"
["set <<ret>> [segment::segment_tcl <<x>> <<y>> <<frame>> <<padding>> <<input_dir>> ]"];

argv_accept("i", "I", "f");
input_dir = argv("I", "./");
file input_f = input(input_dir + argv("i", "log.txt"));
images = file_lines(input_f);

string output[];
foreach image in images {
  printf("array: %s\n", image);
  arr = split(image);
  padding = strlen(arr[0]);
  frame = string2int(arr[0]);
  x = string2int(arr[2]);
  y = string2int(arr[3]);
  ret_data = segment(x, y, frame, padding,
          "/home/vineeth/WS/IIT/fastworm/dataset/che2_HR_nf7/input");
  printf("padding: %d, frame: %d, x: %d, y: %d, area: %d\n",
         padding, ret_data[0], ret_data[1], ret_data[2], ret_data[3]);
  output[frame] = frame + ", " + ret_data[1] + ", " + ret_data[2] + ", " + ret_data[3];

}
file output_f <"output.txt">;
string out = string_join(output, "\n");
output_f=write(out);
