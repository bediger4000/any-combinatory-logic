#Generalized Combinatory Logic Interpreter Examples
## Unusual, unique or bizarre combinatorially complete bases
* Peter Hancock's [AMEN basis](amen.basis).
* The related [{O,A,M,E}](oame.basis) basis. AMEN and OAME are both "arithmetic" bases.
* [{B,C,F,I} basis](bcfi.basis), where <kbd><strong>F</strong> a b &rarr; a a</kbd>, half an Ogre.
* Traditional [{B,T,M,K} basis](btmk.basis).
* Traditional [{B,W,C,K} basis](bwck.basis).
* [{S,N,C} basis](snc.basis), where <kbd><strong>N</strong> a b &rarr; a</kbd>.
* [{B,W,C,N} basis](bwcn.basis), just because.
* Little known [{B',W,K} basis](bpkw.basis), complete with abstraction algorithm.
* [{I,B,C,S} basis](ibcs.basis) &lambda;I basis.
* The sparse [{I,J} basis](ij.basis), another &lambda;I basis, with Church Numerals.
* John Tromp's [9-rule bracket abstraction](tromp.abstraction) algorithm.
* Mayer Goldberg's [D numerals](d_numerals) in {S,K,I} basis.
* [Klein fourgroup](fourgroup) with application as the operation.
* [Scott Numerals](scott.numerals) in an elaborate basis, with elaborate abstraction.

I'm pretty sure that {S, N} is not a basis, less sure that {S, N, T} is
not a basis.  I could not get [Prover9](https://www.cs.unm.edu/~mccune/mace4/) to give me an
expression acting as <strong>C</strong>, <strong>T</strong> or <strong>K</strong>.
{S, N, C} definitely constitutes a combinatorially complete basis.

Even though {S, N} probably isn't a basis, {B,W,C,N} does constitute a basis,
since <kbd><strong>C N</strong></kbd> acts like <kbd><strong>K</strong></kbd>,
even proceeding via head reduction.
