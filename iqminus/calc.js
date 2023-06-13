let resultBox = document.getElementById("result")

let _iota = 0
function resetIota() {
    _iota = 0
}

function iota() {
    return _iota ++
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

function clear() {
    resultBox.innerHTML = ""
}

function error(msg) {
    resultBox.style.color = "var(--color-red)"
    resultBox.innerHTML   = msg

    throw "evaluation error"
}

function output(msg) {
    resultBox.style.color = "var(--color-fg)"
    resultBox.innerHTML  += msg + "\n"
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

function evalMathExpr(row, input) {
	let toks = lex(row, input)
	if (toks.length == 0) {
		output("")
		return
	}

    let result = evalAst(parse(toks))

    if (result.assign) {
    	output(`${result.assign} = ${result.value}`)
    } else
	    output(result)
}

function evalMathExprFromInputField(inputId) {
	clear()

	let lines = document.getElementById(inputId).value.split("\n")
    for (let i = 0; i < lines.length; ++ i)
    	evalMathExpr(i + 1, lines[i])
}
