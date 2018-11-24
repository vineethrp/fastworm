import files;
import io;
import string;
import sys;
@dispatch=WORKER

(int ret[]) segment(int x, int y, int frame, int padding,
                    int mina, int maxa, int swz, string input_dir) "segment" "0.0"
["set <<ret>> [segment::segment_tcl <<x>> <<y>> <<frame>> \
                    <<padding>> <<mina>> <<maxa>> <<swz>> <<input_dir>> ]"];

/*
 * Command line arguements
 * -i : input file
 * -I : Input dir
 * -f : number of frames
 * -m : Minimum valid area
 * -M : Maximum valid area
 * -s : Search window Size
 */
argv_accept("i", "I", "f", "m", "M", "s");
frames = string2int(argv("f", "10"));
mina = string2int(argv("m", "200"));
maxa = string2int(argv("M", "400"));
swz = string2int(argv("s", "200"));

input_dir = argv("I", "./");
file input_f = input(argv("i", "log.txt"));

images = file_lines(input_f);
nr_images = size(images);

int nr_frames;
if (frames > nr_images) {
  nr_frames = nr_images;
} else {
  nr_frames = frames;
}

string output[];
for (int i = 0; i < nr_frames; i = i + 1) {
  image = images[i];
  printf("array: %s\n", image);
  arr = split(image);
  padding = strlen(arr[0]);
  frame = string2int(arr[0]);
  x = string2int(arr[2]);
  y = string2int(arr[3]);
  ret_data = segment(x, y, frame, padding, mina, maxa, swz, input_dir);
  printf("padding: %d, frame: %d, x: %d, y: %d, area: %d\n",
         padding, ret_data[0], ret_data[1], ret_data[2], ret_data[3]);
  output[frame] = frame + ", " + ret_data[1] + ", " + ret_data[2] + ", " + ret_data[3];
}

file output_f <"output.txt">;
string out = string_join(output, "\n");
output_f=write(out);
