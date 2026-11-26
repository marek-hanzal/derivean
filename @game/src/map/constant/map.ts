// Metres across the shorter side of the viewport at the closest zoom. The widest zoom is the planet's span.
export const ClosestSpan = 1000;
// The span the map opens with.
export const OpeningSpan = 20_000;
// Natural-log zoom per pixel of wheel travel: a notch of an ordinary wheel is about a sixth closer or wider.
export const ZoomPerWheelPixel = 0.0015;
// Pixels the pointer may wander between press and release and still count as a click.
export const ClickSlack = 4;
// Device pixels the view has to move before the chart is drawn again.
export const RedrawShift = 0.5;
// A view drawn again within MotionMs is in motion and is drawn at one pixel per CSS pixel; after RestMs without
// moving it is drawn once more at full sharpness.
export const MotionMs = 60;
export const RestMs = 150;

// A chart covers the viewport and this much more, so a short pan does not run off its edge.
export const ChartMargin = 1.3;
// How far the view may leave the chart on screen before another is asked for: its centre by this share of the
// chart's span, its zoom to these ratios of the chart's. Inside the margin above.
export const ChartDrift = 0.1;
export const ChartZoom: [number, number] = [0.72, 1.4];
// The first chart of a new view is this many pixels a side: quick to read, soon replaced.
export const PreviewSize = 240;
// The sharp chart is PaneCount² squares of PaneSize pixels, asked for side by side: each answer stays small
// and the engine reads them in parallel.
export const PaneSize = 256;
export const PaneCount = 3;
// Charts are asked for on a ladder of spans, this many rungs to a doubling.
export const SpanRungs = 4;
// How often an open map asks whether the simulation has finished more tiles.
export const StaleMs = 1500;
// A sharp chart is read only for a view that will still be inside it after this long at its present speed;
// a view that moves faster keeps to coarse charts.
export const SharpWorthMs = 1500;

// °C of annual mean warmth between which the map draws ground from snowed in to clear. It is the map's reading
// of the climate, not the snow lying in the world on a given day.
export const SnowWarmth: [number, number] = [-6, 0];
// Metres of loose cover between which the map lets rock show through.
export const BareCover: [number, number] = [0.15, 0.45];
// Share of a chart's dry pixels that may lie on a drawn stream; smaller streams are left out.
export const StreamShare = 0.035;
// Slope, after exaggeration, that the root-mean-square slope of a chart is shaded as.
export const TypicalShade = 0.2;
// Metres above the surface a traveller is put down at; the engine settles them from there.
export const TravelClearance = 2;
