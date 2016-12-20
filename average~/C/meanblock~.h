/*
// Copyright (c) 2016 Pierre Guillot.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#ifndef MEANBLOCK_H_   /* Include guard */
#define MEANBLOCK_H_



static t_class *meanblock_tilde_class;

typedef struct _meanblock_tilde
{
	t_object    x_obj;

	t_sample*   t_buffer;
	t_int       t_buffer_nrows;
	t_int       t_buffer_ncols;
	t_int       t_crow;
	t_float     t_dummy;
} t_meanblock_tilde;

// Checks if the memory for the buffer has been allocated and frees it if need.
// The method can also be called internally by the object and not only be Pd itself.
void meanblock_tilde_free(t_meanblock_tilde *x)
{
	if (x->t_buffer)
	{
		freebytes(x->t_buffer, x->t_buffer_ncols * x->t_buffer_nrows * sizeof(t_sample *));
		x->t_buffer = NULL;
	}
	x->t_buffer_nrows = 0;
	x->t_buffer_ncols = 0;
}

// Allocates or reallocates the memory for the buffer.
void meanblock_tilde_buffer_alloc(t_meanblock_tilde *x, t_int nrows, t_int ncols, char dspmethod)
{
	t_int i;
	if (nrows && ncols)
	{
		// Instead of using realloc or resizebytes, we free and allocate the memory thus the
		// code is simpler and we avoid a lot of conditions.
		meanblock_tilde_free(x);
		x->t_buffer = (t_sample*)getbytes(nrows * ncols * sizeof(t_sample));
		if (x->t_buffer)
		{
			for (i = 0; i < nrows * ncols; ++i)
			{
				x->t_buffer[i] = 0;
			}
			x->t_buffer_nrows = nrows;
			x->t_buffer_ncols = ncols;
		}
		else
		{
			// If the allocation failded and DSP is running, Pd has to update the dsp and
			// recall our dsp method we can't perform anymore. But if the method has been
			// called within our dsp method it can create a loop so we have to check this.
			if (!dspmethod)
			{
				canvas_update_dsp();
			}
			pd_error(x, "meanblock~ can't allocate memory.");
		}
	}
	else
	{
		// If the allocation failded and DSP is running, Pd has to update the dsp and
		// recall our dsp method we can't perform anymore. But if the method has been
		// called within our dsp method it can create a loop so we have to check this.
		if (!dspmethod)
		{
			canvas_update_dsp();
		}
		pd_error(x, "meanblock~ the numbers of rows and columns are negatives or null.");
	}
}

// For each sample index, computes the average of the samples over a defined number of block.
// For example, for the index i and a number of blocks n, the result of the output vector
// will output[i] = (current[i] + previous[0][i] + previous[1][i] + ... + previous[n-1][i])/n
// See this discussion: http://forum.pdpatchrepo.info/topic/10466/matrices-and-reallocating-memory
t_int *meanblock_tilde_perform(t_int *w)
{
	t_int           i, j;
	t_sample*       cout;
	t_meanblock_tilde *x = (t_meanblock_tilde *)(w[1]);
	t_sample        *in = (t_sample *)(w[2]);
	t_sample        *out = (t_sample *)(w[3]);
	t_int           n = (t_int)(w[4]);

	// The t_buffer member of the object is used to store the input samples. This is a vector
	// in a C matrix form (number of rows * number of columns) so to access the cell at the
	// column i and the row j, like matrix[i][j], you need to compute the position of the cell
	// in a single dimension vector, like vector[i * number of columns + j].
	t_sample* read = x->t_buffer;      // The pointer is used to read the value.
	t_int     nrows = x->t_buffer_nrows;// The number of rows (aka the length).
	t_int     ncols = n;                // The number of columns (aka number of samples).

										// At each new DSP tick, the oldest raw of the matrix is erased and replaced with the
										// samples values of the current input.
	t_int     crow = x->t_crow;           // The row that will be overridden
	t_sample* write = read + crow * ncols; // The pointer is used to write the value.

										   // Records the inputs samples in the row of the matrix
										   // and clears the output vector
	cout = out;
	for (i = 0; i < n; ++i)
	{
		*write++ = *in++;
		*cout++ = 0.f;
	}

	// For each row i of the matrix, adds to the index j of the output vector
	// the value of the column j, like output[j] += matrix[i][j]
	for (i = 0; i < nrows; ++i)
	{
		cout = out;
		for (j = 0; j < n; ++j)
		{
			*cout++ += *read++;
		}
	}

	// Divides the output vector by the number of rows
	cout = out;
	for (i = 0; i < n; ++i)
	{
		*cout++ /= (t_sample)nrows;
	}

	// Increments the next row that will be overriden and if the index is superior or equal
	// to the number of row, go back to zero.
	x->t_crow++;
	if (x->t_crow >= nrows)
	{
		x->t_crow = 0;
	}
	return (w + 5);
}

void meanblock_tilde_dsp(t_meanblock_tilde *x, t_signal **sp)
{
	// Resize the matrix to fit the current number of samples per blocks
	meanblock_tilde_buffer_alloc(x, x->t_buffer_nrows, (t_int)sp[0]->s_n, 1);
	// If the buffer isn't allocated or the number of number of rows or the number of columns
	// are null, we can't perform.
	if (x->t_buffer || x->t_buffer_nrows || x->t_buffer_ncols)
	{
		dsp_add(meanblock_tilde_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
	}
}

void meanblock_tilde_length(t_meanblock_tilde *x, t_floatarg f)
{
	if (f > 0)
	{
		// If the number of column is null, it means that the dsp method hasn't been called
		// yet, so we only need to save the number of rows. Otherwise, we reallocate the
		// memory with the right number of rows.
		if (x->t_buffer_ncols)
		{
			meanblock_tilde_buffer_alloc(x, (t_int)f, x->t_buffer_ncols, 0);
		}
		else
		{
			x->t_buffer_nrows = (t_int)f;
		}
	}
	else
	{
		pd_error(x, "meanblock~ please give me a positive length.");
	}

}

void *meanblock_tilde_new(t_floatarg f)
{
	t_meanblock_tilde *x = (t_meanblock_tilde *)pd_new(meanblock_tilde_class);
	if (x)
	{
		x->t_buffer = NULL;
		x->t_buffer = NULL;
		x->t_buffer_ncols = 0;
		if (f > 0)
		{
			x->t_buffer_nrows = (t_int)f;
		}
		else
		{
			x->t_buffer_nrows = 1;
			pd_error(x, "meanblock~ please give me a positive length.");
		}
		x->t_crow = 0;
		outlet_new(&x->x_obj, &s_signal);
	}
	return x;
}

void init_meanblock(void)
{
	t_class *c = class_new(gensym("meanblock~"), (t_newmethod)meanblock_tilde_new, (t_method)meanblock_tilde_free,
		sizeof(t_meanblock_tilde), CLASS_DEFAULT, A_DEFFLOAT, 0);
	if (c)
	{
		class_addmethod(c, (t_method)meanblock_tilde_dsp, gensym("dsp"), A_CANT, 0);
		class_addmethod(c, (t_method)meanblock_tilde_length, gensym("length"), A_FLOAT, 0);
		CLASS_MAINSIGNALIN(c, t_meanblock_tilde, t_dummy);
	}
	meanblock_tilde_class = c;
}

#endif  // MEANBLOCK_H_