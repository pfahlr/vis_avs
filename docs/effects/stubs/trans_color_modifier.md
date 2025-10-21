# Trans / Color Modifier

The color modifier remaps the RGB channels using trigonometric lookup tables. Three modes are supported:

* **Sine** – applies a smooth sine curve to each color channel for soft contrast.
* **Cosine** – applies a cosine curve, emphasizing mid-tones while reducing extremes.
* **SineCosine** – staggers sine curves with phase offsets across R/G/B for a shifting hue effect.

Select the desired mode with the `mode` parameter. You can pass either the numeric index (`0`, `1`, `2`) or a
string label (e.g. `"sine"`, `"cosine"`, `"sinecos"`).
