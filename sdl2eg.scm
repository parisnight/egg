;;; rendering with guile-sdl2 2020.10.15

(use-modules (sdl2)
             (sdl2 render)
             (sdl2 surface)
(sdl2 rect)
             (sdl2 video))

(define (draw ren)
  (let* ((surface (load-bmp "hello.bmp"))
         (texture (surface->texture ren surface)))

    (clear-renderer ren)
    (render-copy ren texture)

(render-draw-line ren 0 0 50 50)
(render-draw-line ren 0 30 60 150)
(render-draw-line ren 9 30 50 350)
(set-render-draw-color ren 255 0 0 100)
(render-draw-line ren 0 30 50 150)
(render-draw-line ren 0 30 50 180)
(set-render-draw-color ren 255 0 255 100)
(render-draw-rects ren (list (make-rect 0 30 50 180) (make-rect 70 80 200 197)))
(render-fill-rect ren (make-rect 300 300 50 20))
(present-renderer ren)
(sleep 3)
))

(define (go)
  (sdl-init)
  (call-with-window (make-window)
		    (lambda (window)
		      (call-with-renderer (make-renderer window) draw)))
  (sdl-quit))

; (go)
