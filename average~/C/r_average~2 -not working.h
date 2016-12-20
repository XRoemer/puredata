#ifndef AVERAGE_H_   /* Include guard */
#define AVERAGE_H_

static t_class *average_tilde_class;

typedef struct _average_tilde {
	t_object	x_obj;
	t_int		block_size;	

	t_int		len_avg;	// length of dsp ticks on which the samples have to be averaged

	t_sample	*vector;	// all recorded samples (block size * len_avg)
	t_int		row;		// keeps actual calculated row; row is wrapped between 0 <= row <= len_avg
	t_sample	**matrix;	// pointer to vector[row]

	t_sample	*avg;		// holds the average of each sample row

	t_inlet*	x_in;
	t_outlet*	x_out;

} t_average_tilde;

void average_tilde_free (t_average_tilde *x)
{	
	// Deallocate matrix
	if (x->matrix) free(x->matrix);
	// Deallocate vector
	if (x->vector) free(x->vector);
}

t_int *average_tilde_perform(t_int *w)
{	
	t_average_tilde	*x = (t_average_tilde *)(w[1]);
	t_sample		*in = (t_sample *)(w[2]);
	t_sample		*out = (t_sample *)(w[3]);

	t_sample val;

	if (x->matrix) {

		for (int n = 0; n < x->block_size; n++) {

			int t = x->matrix[x->row][n];

			x->avg[n] -= x->matrix[x->row][n];	// Subtract the old value from average
			val = in[n] / x->len_avg;			// New value = in[n] / dsp ticks
			x->avg[n] += val;					// Add the new value to average
			x->matrix[x->row][n] = val;			// Overwrite matrix at row position with new value

			*out++ = x->avg[n];
		}

		x->row++;								// go to next row
		if (x->row == x->len_avg) x->row = 0;
	}

	else *out++ = 0.0;

	return (w + 5);
}

void resize_avg_array(t_average_tilde *x, t_int len_avg_new)
{
	int i,j;
	average_tilde_free(x);

	t_sample *temp_vector = NULL;
	t_sample **temp_matrix = NULL;

	// Allocate the vector
	temp_vector = realloc(temp_vector, len_avg_new * x->block_size * sizeof(t_sample*));

	if (temp_vector) {
		// Allocate the matrix
		temp_matrix = realloc(temp_matrix, len_avg_new * sizeof(t_sample*));

		if (temp_matrix) {
			x->vector = temp_vector;
			x->matrix = temp_matrix;
			x->row = 0;
			x->len_avg = len_avg_new;

			for (int i = 0; i < len_avg_new; i++) {
				x->matrix[i] = x->vector + i * x->block_size;
				for (int j = 0; j < x->block_size; j++) {
					x->matrix[i][j]   = 0.0;
				}
			}
		}
		else {
			free(temp_matrix);
			free(temp_vector);
			x->vector = NULL;
			x->matrix = NULL;
			pd_error(x, "allocation failed");
		}
	}

	else {
		free(temp_vector);
		x->vector = NULL;
		x->matrix = NULL;
		pd_error(x, "allocation failed");
	}
}

void average_tilde_dsp(t_average_tilde *x, t_signal **sp)
{	
	x->block_size = sp[0]->s_n;

	float arr_size = sizeof(x->matrix) / sizeof(x->matrix[0][0]);

	if (x->block_size * x->len_avg != arr_size)
		resize_avg_array(x, 10);

	dsp_add(average_tilde_perform, 4, 
		x,
		sp[0]->s_vec,
		sp[1]->s_vec,
		sp[0]->s_n);	
}

void set_len_avg(t_average_tilde *x, t_floatarg f)
{
	if ((int)f > 0) {
		resize_avg_array(x, f);
	}
}

void init_arrays(t_average_tilde *x, t_floatarg f)
{	
	// initialize values with defaults
	x->len_avg = ((int)f > 0) ? (int)f : 10;
	x->block_size = 64;
	x->row = 0;

	resize_avg_array(x, x->len_avg);

	for (int n = 0; n < x->len_avg; n++)
		for (int m = 0; m < x->block_size; m++)
			x->matrix[n][m] = 0.0;

	// Initialize average-array
	x->avg = realloc(x->avg, x->block_size * sizeof(t_sample));
	for (int i = 0; i < x->block_size; i++)
		x->avg[i] = 0.0;
}

void *average_tilde_new(t_floatarg f)
{
	t_average_tilde *x = (t_average_tilde *)pd_new(average_tilde_class);

	init_arrays(x, f);
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