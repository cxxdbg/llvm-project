! UNSUPPORTED: offload-cuda

! Regression test for a runtime bug in InputNamelist where an empty
! assignment to an item aborted with
!   fatal Fortran runtime error: Bad character 'i' in LOGICAL input field
! The empty value should leave the item at its current value and parsing
! should continue with the next assignment.

! RUN: %flang %isysroot -L"%libdir" %s -o %t
! RUN: env LD_LIBRARY_PATH="$LD_LIBRARY_PATH:%libdir" %t | FileCheck %s

! CHECK: l_flag=F
! CHECK-NEXT: i_count=7
program p
  implicit none
  logical :: l_flag = .false.
  integer :: i_count = 42
  namelist /test_nml/ l_flag, i_count
  character(len=64) :: buf = "&test_nml l_flag= i_count=7 /"

  read(buf, nml=test_nml)

  print '(a,l1)', 'l_flag=', l_flag
  print '(a,i0)', 'i_count=', i_count
end program
