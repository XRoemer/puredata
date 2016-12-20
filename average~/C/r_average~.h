#ifndef AVERAGE_H_   /* Include guard */
#define AVERAGE_H_

// For each sample index, computes the average of the samples over a defined number of blocks.

static t_class *average_tilde_class;

typedef struct _average_tilde {
	t_object	x_obj;
	t_int		block_size;	

	t_int		row;		// tracks the actual row
	t_sample	**matrix;	// records samples of each dsp tick, matrix[row][column]
	t_int		len_avg;	// length of dsp ticks on which the samples have to be averaged
	t_sample	*avg;		// holds the average of matrix' columns
							

	t_inlet*	x_in;
	t_outlet*	x_out;

} t_average_tilde;

void average_tilde_free (t_average_tilde *x)
{	
	// Deallocate rows
	for (int i = 0; i < x->len_avg; ++i)
		free(x->matrix[i]);
	// Deallocate columns
	free(x->matrix);
	// Deallocate avg
	free(x->avg);
}

// Perform Method
// matrix[row] holds the values wich have been added to avg
//
// With each dsp tick:
// Subtract the old values in matrix[row] from avg
// Get the new value (in[n] / len_avg)
// Add value to avg[n]
// Overwrite matrix at row position with new values
//
// See this discussion: http://forum.pdpatchrepo.info/topic/10466/matrices-and-reallocating-memory

t_int *average_tilde_perform(t_int *w)
{	
	t_average_tilde	*x = (t_average_tilde *)(w[1]);
	t_sample		*in = (t_sample *)(w[2]);
	t_sample		*out = (t_sample *)(w[3]);

	t_sample val;

	for (int n = 0; n < x->block_size; n++) {

		x->avg[n] -= x->matrix[x->row][n];	// Subtract the old value from average
		val = in[n] / x->len_avg;			// New value = in[n] / len_avg
		x->avg[n] += val;					// Add the new value to average
		x->matrix[x->row][n] = val;			// Overwrite matrix at row position with new value

		*out++ = x->avg[n];
	}

	x->row++;								// go to next row
	if (x->row == x->len_avg) x->row = 0;	// wrap row pointer 0 <= row < len_avg

	return (w + 5);
}

t_sample **reallocate_matrix(t_average_tilde *x, t_int len_avg_new) {
	int i, j;

	t_sample **temp_matrix = NULL;

	// Allocate the rows
	temp_matrix = realloc(temp_matrix, len_avg_new * sizeof(t_sample*));

	if (temp_matrix) {
		// The new column's pointer must be initialised to NULL
		for (i = 0; i < len_avg_new; i++)
			temp_matrix[i] = NULL;

		// Allocate the columns of each row
		for (i = 0; i < len_avg_new; i++) {
			temp_matrix[i] = realloc(temp_matrix[i], x->block_size * sizeof(t_sample));

			if (temp_matrix[i]) {
				// Initialize the element(s)
				for (j = 0; j < x->block_size; j++)
					temp_matrix[i][j] = 0.0;
			}
			else {
				for (j = 0; j < i; j++)
					free(temp_matrix[j]);
				free(temp_matrix);
				temp_matrix = NULL;
				break;
			}
		}
	}
	return temp_matrix;
}

t_sample *reallocate_avg(t_average_tilde *x, t_int len_avg_new) {

	t_sample *temp_avg = NULL;

	// Allocate avg-array
	temp_avg = realloc(temp_avg, x->block_size * sizeof(t_sample));

	if (temp_avg) {
		// Initialize the element(s)
		for (int i = 0; i < x->block_size; i++)
			temp_avg[i] = 0.0;
	}
	return temp_avg;
}


void resize_avg_array(t_average_tilde *x, t_int len_avg_new, t_int initialized)
{
	int i,j;
	
	t_sample **temp_matrix = NULL;
	t_sample *temp_avg = NULL;

	temp_matrix = reallocate_matrix(x, len_avg_new);

	if (temp_matrix) {
		// Reallocate avg-array
		temp_avg = reallocate_avg(x, len_avg_new);
	}

	if (temp_avg) {
		// assign temps

		// Deallocate columns
		for (i = 0; i < x->len_avg; ++i)
			if (initialized)
				free(x->matrix[i]);
		// Deallocate rows
		free(x->matrix);

		// Assign temps
		x->len_avg = len_avg_new;
		x->matrix = temp_matrix;
		free(x->avg);
		x->avg = temp_avg;
		x->row = 0;
	}
	else {
		free(temp_avg);

		if (temp_matrix) {
			for (i = 0; i < len_avg_new; i++)
				if (initialized)
					free(temp_matrix[i]);
			free(temp_matrix);
			temp_matrix = NULL;
		}
		pd_error(x, "r_avg~ can't allocate memory.");
	}			
}

void average_tilde_dsp(t_average_tilde *x, t_signal **sp)
{	
	if (x->block_size != sp[0]->s_n) {
		x->block_size = sp[0]->s_n;
		resize_avg_array(x, 10, 1);
	}

	dsp_add(average_tilde_perform, 4, 
		x,
		sp[0]->s_vec,
		sp[1]->s_vec,
		sp[0]->s_n);	
}

void set_len_avg(t_average_tilde *x, t_floatarg f)
{
	if ((int)f > 0)
		resize_avg_array(x, f, 1);
}

void *average_tilde_new(t_floatarg f)
{
	t_average_tilde *x = (t_average_tilde *)pd_new(average_tilde_class);

	// initialize values with defaults
	x->len_avg = ((int)f > 0) ? (int)f : 10;
	x->block_size = 64;
	x->row = 0;
	resize_avg_array(x, 10, 0);

	x->x_out = outlet_new(&x->x_obj, &s_signal);

	return (void *)x;
}

void init_average(void) {
	average_tilde_class = class_new(gensym("r_avg~"),
		(t_newmethod)average_tilde_new,
		(t_method)average_tilde_free,
		sizeof(t_average_tilde),
		CLASS_DEFAULT,
		A_DEFFLOAT, 0);

	class_addmethod(average_tilde_class,
		(t_method)average_tilde_dsp, gensym("dsp"), 0);
	CLASS_MAINSIGNALIN(average_tilde_class, t_average_tilde, len_avg);
	class_addfloat(average_tilde_class, set_len_avg);
}
#endif  // AVERAGE_H_