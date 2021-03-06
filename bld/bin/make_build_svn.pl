#!/usr/local/bin/perl
#	\file make_build_svn.pl
#	\brief SVN revision file generator.
#	\author Andriy Golovnya <andriy.golovnya@gmail.com> http://redscorp.net/
#	\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/
#	\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html

$projectroot = '.';
$buildvar = 'CZ_VER_BUILD';
$buildstrvar = 'CZ_VER_BUILD_STRING';
$urlvar = 'CZ_VER_BUILD_URL';

if(@ARGV < 1) {
	$outfile = '&STDOUT';
} else {
	$outfile = $ARGV[0];
}

if(@ARGV > 1) {
	$projectroot = $ARGV[1];
}

open(OUT, '>'.$outfile) or die 'Unable to open output file '.$outfile.'!';
open(VER, '-|', 'svnversion -c -n '.$projectroot) or die 'Can\'t get SVN revision value!';
open(INFO, '-|', 'svn info '.$projectroot) or die 'Can\'t get SVN information!';

print OUT '/*!'."\t".'\\file '.$outfile.''."\n";
print OUT "\t".'\\brief SVN revision definition.'."\n";
print OUT "\t".'\\warning This file is automatically generated by script make_build_svn.pl.'."\n";
print OUT "\t".'\\author Andriy Golovnya <andriy.golovnya@gmail.com> http://redscorp.net/'."\n";
print OUT "\t".'\\url http://cuda-z.sf.net/ http://sf.net/projects/cuda-z/'."\n";
print OUT "\t".'\\license GPLv3 http://www.gnu.org/licenses/gpl-3.0.html'."\n";
print OUT '*/'."\n";
print OUT "\n";
print OUT '#ifndef CZ_BUILD_H'."\n";
print OUT '#define CZ_BUILD_H'."\n";

$line = <VER>;
if($line =~ /exported/) {
	$svnrevision = '0';
	print OUT '/* WARNING! Cannot detect SVN revision number. */'."\n";

} else {
	if($line =~ /:/) {
		$line =~ s/^.*://;
	}
	$svnrevision = $line;
	$svnrevision =~ s/\D//g;

	print OUT "\n";
	print OUT '#define '.$buildvar."\t\t".$svnrevision."\t".'/*!< SVN revision numeric constant. */'."\n";
	print OUT '#define '.$buildstrvar."\t".'"'.$svnrevision.'"'."\t".'/*!< SVN revision string constant. */'."\n";

	$svnurl = 'Unknown';
	while($line = <INFO>) {
		chop $line;
		if($line =~ /^URL: /) {
			$svnurl = $line;
			$svnurl =~ s/^URL: //;
		}
	}

	print OUT '#define '.$urlvar."\t".'"'.$svnurl.'"'."\t".'/*!< SVN repository URL. */'."\n";
}

print OUT "\n";
print OUT '#endif /*CZ_BUILD_H*/'."\n";

close(OUT);
close(VER);
close(INFO);
