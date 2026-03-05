import "lib/github.com/abxh/lys/lys"

type~ lys_state =
  { time: f32
  , h: i64
  , w: i64
  , center: (i64, i64)
  , center_object: #banner | #circle | #square
  , moving: (i64, i64)
  , mouse: (i64, i64)
  , radius: i64
  , paused: bool
  , msg_val: i64
  , triangles: [](f32, f32, f32)
  }

module lys_text = {
  type text_content = (i64, i64, i64, i64, i64, i64, i64, i64, i64)

  def text_format () =
    ""
    ++ "FPS: %ld\n"
    ++ "Center: (%ld, %ld)\n"
    ++ "Center object: %[banner|circle|square]\n"
    ++ "Radius: %ld\n"
    ++ "Size: (%ld,%ld)\n"
    ++ "foo.bin, value: %ld\n"
    ++ "banner.obj, num triangles loaded: %ld"

  def text_content (fps: f32) (s: lys_state) : text_content =
    let center_object_id =
      match s.center_object
      case #banner -> 0
      case #circle -> 1
      case #square -> 2
    in ( i64.f32 fps
       , s.center.0
       , s.center.1
       , center_object_id
       , s.radius
       , s.w
       , s.h
       , s.msg_val
       , length s.triangles / 3
       )

  def text_colour = const argb.yellow
}

module lys_file = {
  def input_file_names () =
    ""
    ++ "foo.bin,"
    ++ "banner.obj,"

  def load_bin [n] (i: i64) (content: [n]u8) (s: lys_state) : lys_state =
    match i
    case 0 ->
      let value = replicate (n) 0
      let value' =
        loop (value) for i < n - 1 do
          value with [i + 1] = 10 * value[i] + i64.u8 (content[i] - '0')
      in s with msg_val = value'[n - 1]
    case _ ->
      s

  def load_obj_vertices [n] (i: i64) (vs: [n](f32, f32, f32)) (s: lys_state) : lys_state =
    match i
    case 1 ->
      s with triangles = vs
    case _ ->
      s

  def load_obj_normals _ _ s = s
  def load_obj_texcoords _ _ s = s
}

module lys : lys with text_content = lys_text.text_content = {
  open lys_text
  open lys_file

  type~ state = lys_state

  def grab_mouse = false

  def init (seed: u32) (h: i64) (w: i64) : lys_state =
    { time = 0
    , w
    , h
    , center = (h / (1 + i64.u32 seed % 11), w / (1 + i64.u32 seed % 7))
    , center_object = #banner
    , moving = (0, 0)
    , mouse = (0, 0)
    , radius = 20
    , paused = false
    , msg_val = 0
    , triangles = replicate 0 (0, 0, 0)
    }

  def resize (h: i64) (w: i64) (s: lys_state) =
    s with h = h with w = w

  def keydown (key: i32) (s: lys_state) =
    if key == SDLK_RIGHT
    then s with moving.1 = 1
    else if key == SDLK_LEFT
    then s with moving.1 = -1
    else if key == SDLK_UP
    then s with moving.0 = -1
    else if key == SDLK_DOWN
    then s with moving.0 = 1
    else if key == SDLK_SPACE
    then s with paused = !s.paused
    else if key == SDLK_b
    then s with center_object = #banner
    else if key == SDLK_c
    then s with center_object = #circle
    else if key == SDLK_s
    then s with center_object = #square
    else s

  def keyup (key: i32) (s: lys_state) =
    if key == SDLK_RIGHT
    then s with moving.1 = 0
    else if key == SDLK_LEFT
    then s with moving.1 = 0
    else if key == SDLK_UP
    then s with moving.0 = 0
    else if key == SDLK_DOWN
    then s with moving.0 = 0
    else s

  def event (e: event) (s: lys_state) =
    let move (x: i64, y: i64) (dx, dy) = (x + dx, y + dy)
    let diff (x1: i64, y1: i64) (x2, y2) = (x2 - x1, y2 - y1)
    in match e
       case #step td ->
         s with time = s.time + (if s.paused then 0 else td)
           with center = move s.center s.moving
       case #wheel {dx = _, dy} ->
         s with radius = i64.max 0 (s.radius + i64.i32 dy)
       case #mouse {buttons, x, y} ->
         s with mouse = (i64.i32 y, i64.i32 x)
           with center = if buttons != 0
           then move s.center (diff s.mouse (i64.i32 y, i64.i32 x))
           else s.center
       case #keydown {key} ->
         keydown key s
       case #keyup {key} ->
         keyup key s

  def render (s: lys_state) =
    let rotate_point (x: f32) (y: f32) (angle: f32) =
      let s = f32.sin angle
      let c = f32.cos angle
      let xnew = x * c - y * s
      let ynew = x * s + y * c
      in (xnew, ynew)
    let in_banner (py: f32, px: f32): bool =
      let (tx, ty) = s.triangles |> map (\(x0, x1, _) -> (x0, x1)) |> unzip
      let tavg = (reduce (+) 0 tx / f32.i64 (length tx), reduce (+) 0 ty / f32.i64 (length ty))
      let t = zip tx ty |> map \(tx, ty) -> (tx - tavg.0, ty - tavg.1)
      let tmin = ((unzip t).0 |> reduce f32.min f32.inf, (unzip t).1 |> reduce f32.min f32.inf)
      let tmax = ((unzip t).0 |> reduce f32.max (-f32.inf), (unzip t).1 |> reduce f32.max (-f32.inf))
      in if px < tmin.0 || px > tmax.0 || py < tmin.1 || py > tmax.1
         then false
         else map (\i -> (t[3 * i], t[3 * i + 1], t[3 * i + 2])) (iota (length t / 3))
              |> map (\((v0x, v0y), (v1x, v1y), (v2x, v2y)) ->
                        let (v0px, v0py) = (px - v0x, py - v0y)
                        let (v1px, v1py) = (px - v1x, py - v1y)
                        let (v2px, v2py) = (px - v2x, py - v2y)
                        let (v1v2x, v1v2y) = (v2x - v1x, v2y - v1y)
                        let (v2v0x, v2v0y) = (v0x - v2x, v0y - v2y)
                        let (v0v1x, v0v1y) = (v1x - v0x, v1y - v0y)
                        let w1 = v1v2x * v1py - v1v2y * v1px
                        let w2 = v2v0x * v2py - v2v0y * v2px
                        let w3 = v0v1x * v0py - v0v1y * v0px
                        let inside_cclockwise = w1 >= 0 && w2 >= 0 && w3 >= 0
                        let inside_clockwise = w1 <= 0 && w2 <= 0 && w3 <= 0
                        in inside_cclockwise || inside_clockwise)
              |> reduce (||) false
    in tabulate_2d s.h
                   s.w
                   (\i j ->
                      let (i', j') = rotate_point (f32.i64 (i - s.center.0)) (f32.i64 (j - s.center.1)) s.time
                      let r = f32.i64 s.radius
                      let inside =
                        match s.center_object
                        case #banner -> in_banner (rotate_point i' j' (f32.pi / 4))
                        case #circle -> f32.sqrt (i' ** 2 + j' ** 2) < f32.i64 s.radius
                        case #square -> i' >= -r && i' < r && j' >= -r && j' < r
                      in if inside
                         then argb.white
                         else if i' > j' then argb.red else argb.blue)
}
