#!/usr/bin/python
# -*- coding: utf-8 -*-
import sys
import re

import math
import cairo
#import pango
#import pangocairo
import random

def parse(file):
    pattern = re.compile("(.*):(\d+):(\d+):(\d+):(\d+)")

    items = []

    for line in file.readlines():
        match = pattern.match(line)

        if not match:
            print('Input file format mismatch.')
            exit(1)
        
        item = (match.group(1), 
                int(match.group(2)), 
                int(match.group(3)), 
                int(match.group(4)), 
                int(match.group(5)))
        items.append(item)
    
    return items
    
def milliseconds(nanoseconds):
    return nanoseconds / 1000000.0
def microseconds(nanoseconds):
    return nanoseconds / 1000.0

def duration(item):
    return item[4] - item[3]

def min_time(items):
    return min([i[3] for i in items])

def max_time(items):
    return max([i[4] for i in items])

def offset_time(item, o):
    n,a,b,c,d = item
    return n,a-o,b-o,c-o,d-o


color_index = {}
def get_color(name):
    global color_index

    if name in color_index:
        return color_index[name]
    else:
        c = (random.random(), random.random(), random.random())
        color_index[name] = c
        return c

def rounded_rect(ctx, x,y,w,h, r):
    if h < r * 2:
        r = h/2
    if w < r * 2:
        r = w/2
    
    ctx.move_to(x, y+r)
    ctx.arc(x+r,y+r, r, math.pi, math.pi * 1.5)
    ctx.line_to(x+w-r, y)
    ctx.arc(x+w-r,y+r, r, math.pi * -0.5, math.pi * 0.0)
    ctx.line_to(x+w, y+h-r)
    ctx.arc(x+w-r, y+h-r, r, math.pi * 0.0, math.pi * 0.5)
    ctx.line_to(x+r, y+h)
    ctx.arc(x+r, y+h-r, r, math.pi * 0.5, math.pi * 1.0)
    #ctx.line_to(x, y+r)
    ctx.close_path()
    ctx.fill()
     
def aligned_text(ctx, txt, fx, fy):
    xb, yb, w, h, xa, ya = ctx.text_extents(txt)

    ctx.rel_move_to(-w*fx, h*fy)
    ctx.show_text(txt)
    
if (len(sys.argv) != 3):
    print('Usage: %s <tracefile> <outfile>' % sys.argv[0])
    exit(1)

tracefile = open(sys.argv[1], 'r')


items = parse(tracefile)
items = [offset_time(i, min_time(items)) for i in items]


inch = 72.0 * 4
mm = inch/25.4

length = math.ceil(10*milliseconds(max_time(items) - min_time(items)))/10

BORDER = 5*mm
AXIS = 0*mm
HEIGHT_PER_BAR = 2.5*mm
LEN_PER_MS = 20*mm

item_names = []
[item_names.append(n) for n,_,_,_,_ in items if not item_names.count(n)]
BAR_CNT = len(item_names)

VHEIGHT, VWIDTH = BAR_CNT * HEIGHT_PER_BAR,  LEN_PER_MS * length


HEIGHT, WIDTH = VHEIGHT + 2*BORDER + AXIS, VWIDTH + 2*BORDER

surface = cairo.PDFSurface (sys.argv[2], WIDTH, HEIGHT)
ctx = cairo.Context(surface)
#pctx = pangocairo.CairoContext(ctx)

ctx.set_line_width(0.005)

ctx.translate(BORDER, BORDER+AXIS)
ctx.scale(LEN_PER_MS, LEN_PER_MS)

ctx.set_font_size(0.05)
ctx.select_font_face('FreeSans')

for t in range(0, int(length*10)):


    ctx.stroke()
    if t == 166:
        ctx.set_source_rgb(1,0,0)
    elif t % 10 == 0:
        ctx.set_source_rgb(0.75,0.75,0.75)
    elif t % 5 == 0:
        ctx.set_source_rgb(0.87,0.87,0.87)
    else:
        ctx.set_source_rgb(0.95,0.95,0.95)
    ctx.move_to(t * 0.1, -0)
    ctx.rel_line_to(0, VHEIGHT/LEN_PER_MS)
    ctx.stroke()
    ctx.set_source_rgb(0.3,0.3,0.3)

    l = 0.01;
    if t % 10 == 0:
        l = 0.05
    elif t % 5 == 0:
        l = 0.025

    ctx.move_to(t * 0.1, -0)
    ctx.rel_line_to(0, -l)

    ctx.move_to(t * 0.1, VHEIGHT/LEN_PER_MS)
    ctx.rel_line_to(0,  l)

    if t % 10 == 0:
        txt = '%d ms' % (t/10)
        ctx.move_to(t * 0.1, -0.1)
        aligned_text(ctx, txt, 0.5, 0)
        ctx.move_to(t * 0.1, VHEIGHT/LEN_PER_MS+0.1)
        aligned_text(ctx, txt, 0.5, 1)

ctx.set_source_rgb(0.3,0.3,0.3)

ctx.move_to(0, VHEIGHT/LEN_PER_MS)
ctx.rel_line_to(length, 0)

ctx.move_to(0,-0)
ctx.rel_line_to(length, 0)

ctx.stroke()

for v in items:
    n,a,b,c,d = v

    i = item_names.index(n)

    i = BAR_CNT - i - 1

    color = get_color(n)
    
    start = milliseconds(c)
    end = milliseconds(d)

    y = (i + 0.125) * HEIGHT_PER_BAR / LEN_PER_MS
    h = 0.75 * HEIGHT_PER_BAR / LEN_PER_MS

    ctx.set_source_rgb(*color)
    rounded_rect(ctx, start, y, end-start, h, 0.125 * HEIGHT_PER_BAR/LEN_PER_MS)

    _, _, w, _, _, _ = ctx.text_extents(n)
    p = 0.05
    wb = end-start
    if w < wb - 2*p and w > wb/2:
        ctx.set_source_rgb(1,1,1)
        ctx.move_to(start + wb/2, y + h/2)
        aligned_text(ctx, n, 0.5, 0.5)
    elif w < wb - 2*p:
        ctx.set_source_rgb(1,1,1)
        ctx.move_to(start + p, y + h/2)
        aligned_text(ctx, n, 0.0, 0.5)
    else:
        ctx.set_source_rgb(0,0,0)
        ctx.move_to(end   + 0.05, y + h/2)
        aligned_text(ctx, n, 0.0, 0.5)

surface.finish()

# for item in items:
#     print ('%s: %.1f µs' % (item[0], microseconds(duration(item))))

# print ('')

# print ('total runtime: %.1f µs' % microseconds(sum([duration(i) for i in items])))
# print ('from %.1f to %.1f µs (%.1f)' % (microseconds(min_time(items)), 
#                                         microseconds(max_time(items)),
#                                         microseconds(max_time(items) - min_time(items))))
                                        
