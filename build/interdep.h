#ifndef INTERDEP_H
#define INTERDEP_H

/* Perform inter-package analysis and optimizations. */
int processInterdep(Spec spec);
/* Replace NEVR-based inter-package dependencies with Identity-based */
int upgradeInterdep(Spec spec, const char * strict_interdeps);

#endif
