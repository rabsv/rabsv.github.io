// --- MATHCODE ---
let _iota = 0
function resetIota() {
    _iota = 0
}

function iota() {
    return _iota ++
}

function error(_) {
	throw "evaluation error"
}

resetIota()
const Token = {
    Type: {
        Id:      iota(),
        Num:     iota(),
        Add:     iota(),
        Sub:     iota(),
        Mult:    iota(),
        Div:     iota(),
        Mod:     iota(),
        Pow:     iota(),
        LParen:  iota(),
        RParen:  iota(),
        LSquare: iota(),
        RSquare: iota(),
        Pipe:    iota(),
        Equals:  iota(),
    },

    new: (type, data, row, col) => {
        return {
            type: type,
            data: data,
            row:  row,
            col:  col,
        }
    },
}

function isDigit(char) {
    return char.match(/[0-9]/i)
}

function isLetter(char) {
    return char.match(/[a-z]/i) || char == "π"
}

function isWhitespace(char) {
    switch (char) {
    case " ": case "\r": case "\t": case "\n": return true

    default: return false
    }
}

function nextChar(ctx) {
    ++ ctx.pos
    ++ ctx.col
}

function lexNum(ctx) {
    let tok = Token.new(Token.Type.Num, "", ctx.row, ctx.col)

    while (ctx.pos < ctx.input.length && isDigit(ctx.input[ctx.pos])) {
        tok.data += ctx.input[ctx.pos]
        nextChar(ctx)
    }

    return tok
}

function lexId(ctx) {
    let tok = Token.new(Token.Type.Id, "", ctx.row, ctx.col)

    while (ctx.pos < ctx.input.length && isLetter(ctx.input[ctx.pos])) {
        tok.data += ctx.input[ctx.pos]
        nextChar(ctx)
    }

    return tok
}

function lexSingleChar(ctx, type) {
    let tok = Token.new(type, ctx.input[ctx.pos], ctx.row, ctx.col)
    nextChar(ctx)
    return tok
}

function lex(row, input) {
    let ctx = {
        pos:   0,
        row:   row,
        col:   1,
        input: input,
    }

    let toks = []

    while (ctx.pos < ctx.input.length) {
        let ch = ctx.input[ctx.pos]

        switch (ch) {
        case '+':
            toks.push(lexSingleChar(ctx, Token.Type.Add))
            break

        case '-':
            toks.push(lexSingleChar(ctx, Token.Type.Sub))
            break

        case '*':
            toks.push(lexSingleChar(ctx, Token.Type.Mult))
            break

        case '/':
            toks.push(lexSingleChar(ctx, Token.Type.Div))
            break

        case '%':
            toks.push(lexSingleChar(ctx, Token.Type.Mod))
            break

        case '^':
            toks.push(lexSingleChar(ctx, Token.Type.Pow))
            break

        case '(':
            toks.push(lexSingleChar(ctx, Token.Type.LParen))
            break

        case ')':
            toks.push(lexSingleChar(ctx, Token.Type.RParen))
            break

        case '[':
            toks.push(lexSingleChar(ctx, Token.Type.LSquare))
            break

        case ']':
            toks.push(lexSingleChar(ctx, Token.Type.RSquare))
            break

        case '|':
            toks.push(lexSingleChar(ctx, Token.Type.Pipe))
            break

        case '=':
            toks.push(lexSingleChar(ctx, Token.Type.Equals))
            break

        default:
            if (isDigit(ch))
                toks.push(lexNum(ctx))
            else if (isLetter(ch))
                toks.push(lexId(ctx))
            else if (!isWhitespace(ch))
                error(`At ${ctx.row}:${ctx.col}: lex(): Unexpected character "${ctx.input[ctx.pos]}"`)
            else
                nextChar(ctx)
        }
    }

    return toks
}

resetIota()
const Node = {
    Type: {
        Id:     iota(),
        Fun:    iota(),
        Num:    iota(),
        Bin:    iota(),
        Neg:    iota(),
        Abs:    iota(),
        Assign: iota(),
    },

    new: (tok, type) => {
        return {
            tok:  tok,
            type: type,
        }
    },
}

let funs = {
	"sin":   (x) => {return Math.sin(x)},
	"cos":   (x) => {return Math.cos(x)},
	"tan":   (x) => {return Math.tan(x)},
	"floor": (x) => {return Math.floor(x)},
	"ceil":  (x) => {return Math.ceil(x)},
	"round": (x) => {return Math.round(x)},
	"sqrt":  (x) => {return Math.sqrt(x)},
	"cbrt":  (x) => {return Math.cbrt(x)},
	"ln":    (x) => {return Math.log(x)},
}

function parseAtom(ctx) {
    let tok = ctx.toks[ctx.pos]

    if (!tok)
        error(`At [${ctx.pos}]: parseAtom(): Expected token`)

    if (tok.type == Token.Type.Sub) {
        ++ ctx.pos

        let node = Node.new(tok, Node.Type.Neg)
        node.expr = parseAtom(ctx)
        return node
    } else if (tok.type == Token.Type.LParen) {
        ++ ctx.pos
        let expr = parseExpr(ctx)
        tok = ctx.toks[ctx.pos]
        if (tok.type != Token.Type.RParen)
            error(`At ${tok.row}:${tok.col} [${ctx.pos}]: parseAtom(): Expected matching ")", got "${tok.data}"`)
        ++ ctx.pos
        return expr
    } else if (tok.type == Token.Type.LSquare) {
        ++ ctx.pos
        let expr = parseExpr(ctx)
        tok = ctx.toks[ctx.pos]
        if (tok.type != Token.Type.RSquare)
            error(`At ${tok.row}:${tok.col} [${ctx.pos}]: parseAtom(): Expected matching "]", got "${tok.data}"`)
        ++ ctx.pos
        return expr
    } else if (tok.type == Token.Type.Pipe) {
        let start = tok
        ++ ctx.pos
        let expr = parseExpr(ctx)
        tok = ctx.toks[ctx.pos]
        if (tok.type != Token.Type.Pipe)
            error(`At ${tok.row}:${tok.col} [${ctx.pos}]: parseAtom(): Expected matching "|", got "${tok.data}"`)
        ++ ctx.pos
        let node = Node.new(start, Node.Type.Abs)
        node.expr = expr
        return node
    } else if (tok.type == Token.Type.Id) {
        ++ ctx.pos

        if (funs[tok.data]) {
            if (ctx.toks[ctx.pos] && ctx.toks[ctx.pos].type == Token.Type.LParen) {
                let node = Node.new(tok, Node.Type.Fun)
                node.expr = parseAtom(ctx)
                return node
            } else
                error(`At ${tok.row}:${tok.col} [${ctx.pos}]: parseAtom(): "${tok.data}" is a function, expected a parameter`)
        }

        return Node.new(tok, Node.Type.Id)
    } else if (tok.type == Token.Type.Num) {
        ++ ctx.pos
        return Node.new(tok, Node.Type.Num)
    } else
        error(`At ${tok.row}:${tok.col} [${ctx.pos}]: parseAtom(): Unexpected token "${tok.data}"`)
}

function parsePow(ctx) {
    let left = parseAtom(ctx)

    while (ctx.pos < ctx.toks.length && ctx.toks[ctx.pos].type == Token.Type.Pow) {
        let tok = ctx.toks[ctx.pos]
        ++ ctx.pos

        let right = parsePow(ctx)
        let bin   = Node.new(tok, Node.Type.Bin)

        bin.left  = left
        bin.right = right

        left = bin
    }

    return left
}

function parseMultDivMod(ctx) {
    let left = parsePow(ctx)

    while (ctx.pos < ctx.toks.length) {
        let tok = ctx.toks[ctx.pos]

        if (ctx.toks[ctx.pos].type == Token.Type.Mult ||
            ctx.toks[ctx.pos].type == Token.Type.Div  ||
            ctx.toks[ctx.pos].type == Token.Type.Mod) {
            ++ ctx.pos
        } else if (ctx.toks[ctx.pos].type == Token.Type.Id      ||
                    ctx.toks[ctx.pos].type == Token.Type.Num     ||
                    ctx.toks[ctx.pos].type == Token.Type.LSquare ||
                    ctx.toks[ctx.pos].type == Token.Type.LParen) {
            tok = Token.new(Token.Type.Mult, "*", tok.row, tok.col)
        } else
            break

        let right = parsePow(ctx)
        let bin   = Node.new(tok, Node.Type.Bin)

        bin.left  = left
        bin.right = right

        left = bin
    }

    return left
}

function parseAddSub(ctx) {
    let left = parseMultDivMod(ctx)

    while (ctx.pos < ctx.toks.length &&
            (ctx.toks[ctx.pos].type == Token.Type.Add ||
            ctx.toks[ctx.pos].type == Token.Type.Sub)) {
        let tok = ctx.toks[ctx.pos]
        ++ ctx.pos

        let right = parseMultDivMod(ctx)
        let bin   = Node.new(tok, Node.Type.Bin)

        bin.left  = left
        bin.right = right

        left = bin
    }

    return left
}

function parseAssign(ctx) {
    let left = parseAddSub(ctx)

    while (ctx.pos < ctx.toks.length && ctx.toks[ctx.pos].type == Token.Type.Equals) {
        let tok = ctx.toks[ctx.pos]
        ++ ctx.pos

        let right = parseAddSub(ctx)
        let bin   = Node.new(tok, Node.Type.Assign)

        if (left.type != Node.Type.Id)
        	error(`At ${tok.row}:${tok.col} [${ctx.pos}]: parseAssign(): Left side of assignment expected identifier`)

        bin.id   = left
        bin.expr = right

        left = bin
    }

    return left
}

function parseExpr(ctx) {
    return parseAssign(ctx)
}

function parse(toks) {
    let ctx = {
        toks: toks,
        pos:  0,
    }

    let ast = parseExpr(ctx)
    if (ctx.pos < ctx.toks.length)
        error(`At [${ctx.pos}]: parse(): Expected operator`)

    return ast
}

function evalAst(ast) {
    switch (ast.type) {
    case Node.Type.Num: return parseInt(ast.tok.data)
    case Node.Type.Bin: {
        let left = evalAst(ast.left), right = evalAst(ast.right)

        switch (ast.tok.type) {
        case Token.Type.Add:  return left + right
        case Token.Type.Sub:  return left - right
        case Token.Type.Mult: return left * right
        case Token.Type.Div:  return left / right
        case Token.Type.Mod:  return left % right
        case Token.Type.Pow:  return Math.pow(left, right)
        }
    } break

    case Node.Type.Abs: return Math.abs(evalAst(ast.expr))
    case Node.Type.Neg: return -evalAst(ast.expr)

    case Node.Type.Fun: {
        let num = evalAst(ast.expr)

        return funs[ast.tok.data](num)
    } break

    case Node.Type.Assign: {
        let num = evalAst(ast.expr)
        vars[ast.id.tok.data] = num
        return {assign: ast.id.tok.data, value: num}
    } break

    case Node.Type.Id: {
        let var_ = vars[ast.tok.data]

        if (var_ != undefined)
            return var_
        else
            error(`At ${ast.tok.row}:${ast.tok.col}: evalAst(): Unknown identifier "${ast.tok.data}"`)
    } break

    default: error(`At ${ast.tok.row}:${ast.tok.col}: evalAst(): Unknown node`)
    }
}

let vars = {
    "PI": Math.PI,
    "pi": Math.PI,
    "e":  Math.E,
    "π":  Math.PI,
}

function evalMathExpr(input) {
	try {
		let toks = lex(0, input)
		if (toks.length == 0)
			return 0

		return evalAst(parse(toks))
	} catch {
		return input
	}
}

// --- WAVECODE ---

const scale = 60
let on = false
let ctx, waves = [], wavesElem = document.getElementById("waves")

function waveAsHtml(wave) {
	return `
<div class="wave">
	<div class="flex">
		<p>Frequency (Hz)</p>
		<input type="text" class="textline tfreq" value="FREQ">
		<input type="range" min="200" max="3000" value="FREQ" class="slider freq">
		<div class="bord">
			<button class="controller ram remove" onclick="remove(this)"><i class="icon fa fa-remove fa-sm"></i></button>
		</div>
	</div>
	<div class="flex">
		<p>Volume (0 - 100)</p>
		<input type="text" class="textline tvol" value="VOL">
		<input type="range" min="0" max="1" value="VOL" class="slider vol" step="0.01">
		<div class="bord">
			<button class="controller ram" style="opacity: 0"><i class="icon fa fa-remove fa-sm"></i></button>
		</div>
	</div>
	<canvas width="800px" height="150px" class="canv graph"></canvas>
</div>
`.replace(/FREQ/g, wave.freq).replace(/VOL/g, wave.vol)
}

function setWaveFreq(idx, freq) {
	waves[idx].freq = freq
	if (waves[idx].o != undefined)
		waves[idx].o.frequency.value = freq

	let slider = document.getElementsByClassName("freq")[idx]
	if (slider.value != freq)
		slider.value = freq

	let text = document.getElementsByClassName("tfreq")[idx]
	if (text.value != freq)
		text.value = freq
}

function setWaveVol(idx, vol) {
	waves[idx].vol = vol
	waves[idx].g.gain.value = vol

	let slider = document.getElementsByClassName("vol")[idx]
	if (slider.value != vol)
		slider.value = vol

	let text = document.getElementsByClassName("tvol")[idx]
	if (text.value != vol)
		text.value = vol
}

function updateSum() {
	let canv = document.getElementById("sum")
	let ctx  = canv.getContext("2d")
	const w  = canv.width, h = canv.height

	ctx.clearRect(0, 0, w, h)
	ctx.beginPath()
	ctx.moveTo(0, h/2)
	ctx.strokeStyle = "#357ff5"
	for (let i = 0; i < w; ++ i) {
		let sum = 0
		for (let j in waves) {
			let wave = waves[j]
			sum += Math.sin(i / w * Math.PI * (wave.freq/scale)) * wave.vol
		}
		ctx.lineTo(i, h/2 - sum * h/4)
	}
	ctx.stroke()
}

function getIdx(elem, class_) {
	let elems = document.getElementsByClassName(class_)
	for (let i in elems) {
		if (elem == elems[i])
			return i
	}
	return undefined
}

function updateGraph(canv) {
	let ctx  = canv.getContext("2d")
	const w  = canv.width, h = canv.height

	const idx = getIdx(canv, "graph")
	if (waves[idx] == undefined) return

	ctx.clearRect(0, 0, w, h)
	ctx.beginPath()
	ctx.moveTo(0, h/2)
	ctx.strokeStyle = "#357ff5"
	for (let i = 0; i < w; ++ i) {
		ctx.lineTo(i, h/2 - Math.sin(i / w * Math.PI * (waves[idx].freq/scale)) * waves[idx].vol * h/4)
	}
	ctx.stroke()
	updateSum()
}

function addWave(freq, vol) {
	if (ctx == undefined)
		ctx = new AudioContext()

	let wave = {freq: freq, vol: vol, g: ctx.createGain()}
	wave.g.gain.value = vol
	wave.g.connect(ctx.destination)

	waves.push(wave)
	let section = document.createElement("section")
	let header  = document.createElement("header")
	header.className = "fadeable"
	section.innerHTML = waveAsHtml(wave)
	section.appendChild(header)
	wavesElem.appendChild(section)
	window.setTimeout(function() {
		header.className += " fade"
	}, 50)
	window.setTimeout(() => {header.remove()}, 750)

	let canv = section.getElementsByClassName("graph")[0]
	updateGraph(canv)

	let sliderFreq = section.getElementsByClassName("freq")[0]
	sliderFreq.addEventListener('input', () => {
		const idx = getIdx(canv, "graph")
		if (waves[idx] == undefined) return
		setWaveFreq(idx, sliderFreq.value)
		updateGraph(canv)
	}, false);

	let sliderVol = section.getElementsByClassName("vol")[0]
	sliderVol.addEventListener('input', () => {
		const idx = getIdx(canv, "graph")
		if (waves[idx] == undefined) return
		setWaveVol(idx, sliderVol.value)
		updateGraph(canv)
	}, false);

	let textFreq = section.getElementsByClassName("tfreq")[0]
	textFreq.onchange = () => {
		const idx = getIdx(canv, "graph")
		if (waves[idx] == undefined) return
		setWaveFreq(idx, evalMathExpr(textFreq.value))
		updateGraph(canv)
	}

	let textVol = section.getElementsByClassName("tvol")[0]
	textVol.onchange = () => {
		const idx = getIdx(canv, "graph")
		if (waves[idx] == undefined) return
		setWaveVol(idx, evalMathExpr(textVol.value))
		updateGraph(canv)
	}

	if (on) {
		toggle(document.getElementsByClassName("toggle")[0], false)
		toggle(document.getElementsByClassName("toggle")[0], true)
	}
}

function recreateOscs() {
	for (let i in waves) {
	}
}

function toggle(elem, tggl) {
	on = tggl != null? tggl : !on;
	elem.innerHTML = `<i class="icon fa fa-` + (on? "pause" : "play") + ` fa-sm"></i>`

	for (let i in waves) {
		if (!on) {
			if (waves[i].o != undefined) {
				waves[i].o.stop()
				delete waves[i].o
				waves[i].o = undefined
			}
		} else {
			waves[i].o = ctx.createOscillator()
			waves[i].o.type = "sine"
			waves[i].o.frequency.value = waves[i].freq
			waves[i].o.connect(waves[i].g)
			waves[i].o.start()
		}
	}
}

let debounce = false
function remove(elem) {
	if (debounce) return
	debounce = true
	const idx = getIdx(elem, "remove")
	if (waves[idx] == undefined) return

	if (on && waves[idx].o != undefined) {
		waves[idx].o.stop()
		delete waves[idx].o
	}

	let header = document.createElement("header")
	let section = document.getElementsByTagName("section")[idx]
	header.className = "fadeable2"
	section.appendChild(header)
	window.setTimeout(function() {
		header.className += " fade2"
	}, 50)
	window.setTimeout(function() {
		debounce = false
		waves.splice(idx, 1)
		elem.parentElement.parentElement.parentElement.parentElement.remove()
		updateSum()
	}, 750)
}
