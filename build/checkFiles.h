#ifndef CHECKFILES_H
#define CHECKFILES_H

/* Perform filst list check. */
int checkFiles(Spec spec);

/* Helper function: process files with the same name. */
void fiIntersect(TFI_t fi1, TFI_t fi2, void (*cb)(char *f, int i1, int i2));

#endif
