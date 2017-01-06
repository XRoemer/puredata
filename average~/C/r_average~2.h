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
	// Deallocate avg
	if (x->avg) free(x->avg);
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

void average_resize_avg(t_average_tilde *x) {

	t_sample *temp_avg;

	// Allocate avg-array
	temp_avg = malloc(x->block_size * sizeof(t_sample));

	if (temp_avg) {
		// Initialize the element(s)
		for (int i = 0; i < x->block_size; i++)
			temp_avg[i] = 0.0;
	}

	if (x->avg) free(x->avg);
	x->avg = temp_avg;
}

void average_resize_matrix(t_average_tilde *x, t_int len_avg_new)
{
	int i,j;

	t_sample *temp_vector;
	t_sample **temp_matrix;

	// Allocate the vector
	temp_vector = malloc(len_avg_new * x->block_size * sizeof(t_sample*));

	if (temp_vector) {
		// Allocate the matrix
		temp_matrix = malloc(len_avg_new * sizeof(t_sample*));

		if (temp_matrix) {
			for (int i = 0; i < len_avg_new; i++) {
				temp_matrix[i] = temp_vector + i * x->block_size;
				for (int j = 0; j < x->block_size; j++) {
					temp_matrix[i][j] = 0.0;
				}
			}
		}
		else {
			free(temp_matrix);
			free(temp_vector);
			temp_vector = NULL;
			temp_matrix = NULL;
		}
	}

	else {
		free(temp_vector);
		temp_vector = NULL;
	}

	if (x->matrix) free(x->matrix);
	if (x->vector) free(x->vector);
	x->vector = temp_vector;
	x->matrix = temp_matrix;
}

void average_resize_arrays(t_average_tilde *x, t_floatarg f) {

	x->row = 0;
	x->len_avg = f;

	average_resize_avg(x, f);
	average_resize_matrix(x, f);

	if (!x->avg || !x->matrix) {
		if (x->matrix) free(x->matrix);
		if (x->vector) free(x->vector);
		x->matrix = NULL;
		x->vector = NULL;
		pd_error(x, "allocation failed");
	}
}

void average_tilde_dsp(t_average_tilde *x, t_signal **sp)
{	
	if (x->block_size != sp[0]->s_n) {
		x->block_size = sp[0]->s_n;
		average_resize_arrays(x, x->len_avg);
	}

	dsp_add(average_tilde_perform, 4, 
		x,
		sp[0]->s_vec,
		sp[1]->s_vec,
		sp[0]->s_n);	
}

void average_set_len_avg(t_average_tilde *x, t_floatarg f)
{
	if ((int)f > 0) {
		average_resize_arrays(x, (int)f);
	}
}

void average_init_arrays(t_average_tilde *x, t_floatarg f)
{	
	// initialize values with defaults
	x->len_avg = ((int)f > 0) ? (int)f : 10;
	x->block_size = 64;
	x->row = 0;
	x->matrix = NULL;
	x->vector = NULL;
	average_resize_arrays(x, x->len_avg);
	post("hier");
}

void *average_tilde_new(t_floatarg f)
{
	t_average_tilde *x = (t_average_tilde *)pd_new(average_tilde_class);

	average_init_arrays(x, f);
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
	class_addfloat(average_tilde_class, average_set_len_avg);
}
#endif  // AVERAGE_H_