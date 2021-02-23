!
! Copyright (c) 2019, Advanced Micro Devices, Inc. All rights reserved.
!
! F2008 Compliance Tests: G0 Edit descriptor - Input/Output extensions
!
! Date of Modification: 31st Aug 2019
!
! Tests if the G0 Edit descriptors work correctly
PROGRAM EDITD02
	IMPLICIT NONE
	INTEGER, PARAMETER :: N = 1
  LOGICAL EXP(N), RES(N)
  REAL A, B, C, D

  A = 3.14159
  B = -99.8
  C = 321.4567E-02
  D = 5.99392558D+08
  WRITE(*,100)A,B,C,D
100 FORMAT(' ',4G10.2)
CALL CHECK(RES, EXP, N)
END PROGRAM
