#!/bin/perl

use File::Temp qw/ tempdir /;


# ////////////////////////////////////////////////////////////////////////
# Check args

if (@ARGV != 1) {
  die "Syntax: build <version>\n";
}
$version = $ARGV[0];


# ////////////////////////////////////////////////////////////////////////
# Main code

# Build the solution
BuildSolution("pdbdump.sln", "Release");


# Make up the bin tgz
$moduleName = "pdbdump-w32";
MakeTemp();
CopyFiles("Release/pdbdump.exe", "bin");
CopyFiles("LICENSE.TXT", "ReadMe.txt", "");
MakeTgz("pdbdump-w32", $version);

# Make up the source tgz
$moduleName = "pdbdump-src";
MakeTemp();
CopyFiles("*.h", "*.cpp", "*.sln", "*.vcproj", "");
CopyFiles("LICENSE.TXT", "ReadMe.txt", "Compiling.txt", "");
MakeTgz("pdbdump-src", $version);


# All done
CleanUp();
print "Build successful\n";
exit;





# Build a solution
#
# @param solutionName Name of the SLN file to build
# @param target Solution target to build (e.g. "Release" etc)
sub BuildSolution {
  my ($solutionFilename, $target) = @_;
  
  # do it
  system("devenv.exe $solutionFilename /build $target") and die ("Unable to build solution $solutionFilename/$target\n");
}


# Make a temporary directory
sub MakeTemp {
  if ($tempDir eq "") {
    $tempDir = tempdir();
  } else {
    system("rm -fr $tempDir");
  }
}


# Copy files
#
# @param List of files to copy
# @lastparam Destination
sub CopyFiles {
  my (@files) = @_;
  
  # Work out the destination
  my $dst = "$tempDir/$moduleName/" . pop(@files);
  if ($dst eq "") {
    $dst = "$tempDir/$moduleName";
  }
  
  # Create the destination directory if necessary
  if (! -e $dst) {
    system("mkdir -p $dst") and Error("Failed to create directory $dst\n");
  } elsif (! -d $dst) {
    die "CopyFiles destination $dst exists, but is not a directory.\n";
  }
  
  # Copy the files across
  foreach $file (@files) {
    system("cp -fr $file $dst") and Error("Failed to copy file $file to $dst\n");
  }
}


# Delete files
#
# @param List of files to be deleted
sub DeleteFiles {
  my @files = @_;
  
  # Delete the files
  foreach $file (@files) {
    $file = "$tempDir/$moduleName/$file";
    system "rm -fr $file" and Error("Failed to delete $file\n");
  }
}


# make up a tgz file
sub MakeTgz {
  my ($baseName, $buildVersion) = @_;
  
  $tgz = "$baseName-$buildVersion.tgz";
  system "tar zcf $tgz -C $tempDir $moduleName" and Error("Failed to create $tgz\n");
}


# Clean everything up again
sub CleanUp {
  system "rm -fr $tempDir";
}


# Report an error and quit
sub Error {
  my @error = @_;

  CleanUp();
  die join(" ", @error) . "\n";
}