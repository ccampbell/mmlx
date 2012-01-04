#!perl

use warnings;
use strict;
use File::Spec;
use File::Copy;
use File::Basename;
my $base_dir = dirname(dirname(File::Spec->rel2abs($0)));
#$base_dir = "/usr/local";
my $mckc = File::Spec->catfile($base_dir, "bin", "ppmckc.exe");
my $nesasm = File::Spec->catfile($base_dir, "bin", "nesasm.exe");
my $inc = File::Spec->catdir($base_dir, "nes_include");
$ENV{'NES_INCLUDE'} = File::Spec->catfile($inc);
my $incasm = File::Spec->catfile($inc, "ppmck.asm");


if (@ARGV < 1) {
	print "too few args\n";
	exit;
}

#absolute path
my $file = $ARGV[0];
if (!-f $file) {
	print "$file: no such file\n";
	exit;
}

my ($basename, $mml_path, $mml_suffix) = fileparse($file, qr{\.(mml|mck|mus)}i);
chdir $mml_path or die $!;
my $mml_name = File::Spec->abs2rel($file);


unlink("effect.h") or die $! if (-f "effect.h");

my $str = "\"$mckc\" -i $mml_name songdata.h";
system($str) and die $!;

exit if (!-f "effect.h");
exit if (!-f "songdata.h");
exit if (!-f "define.inc");


open my $outfh, ">$basename.asm" or die $!;
my @content;
my $fh;

open $fh, "define.inc" or die $!;
{
	local $/;
	print {$outfh} <$fh>;
}
close $fh;


open $fh, $incasm or die $!;
@content = <$fh>;
print {$outfh} grep{!/effect\.h|define\.inc/} @content;
close $fh;

open $fh, "effect.h" or die $!;
@content = <$fh>;
close $fh;

print {$outfh} map{
			if (/^\s+\.?include\s+"songdata\.h"/){
				open $fh, "songdata.h" or die $!;
				{
					local $/;
					$_ = <$fh>;
				}
				close $fh;
			}
			$_;
		} @content;


unlink("effect.h") or die $!;
unlink("songdata.h") or die $!;
unlink("define.inc") or die $!;

unlink("$basename.nes") or die $! if (-f "$basename.nes");

$str = "\"$nesasm\" -s -raw $mml_name $basename.asm";
system($str) and die $!;

exit if (!-f "$basename.nes");
rename "$basename.nes", "$basename.nsf" or die $!;

#system "start $basename.nsf" and die $!;


