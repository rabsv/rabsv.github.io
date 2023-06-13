function trigImage() {
	let unitSwitcher = document.getElementById("unit-switcher")
	let unit         = "Degrees"

	unitSwitcher.onclick = () => {
		if (unit == "Degrees") {
			unit = "Radians"
			unitSwitcher.innerHTML = "Radiány"
		} else {
			unit = "Degrees"
			unitSwitcher.innerHTML = "Stupne"
		}

		update()
	}

	let mousePos = {x: 0, y: 0}

	let canvas = document.getElementById("trig-image")
	let ctx    = canvas.getContext("2d")

	let cosInfo = document.getElementById("cos-info")
	let sinInfo = document.getElementById("sin-info")
	let tanInfo = document.getElementById("tan-info")

	const w = canvas.width, h = canvas.height
	const r = w / 2

	let offset = 20

	let circleW = w - offset * 2, circleH = h - offset * 2
	let circleR = r - offset

	const colorCircle   = "#c850c8"
	const colorLines    = "#999999"
	const colorSin      = "#357ff5"
	const colorCos      = "#00c800"
	const colorTan      = "#f8a800"
	const colorTriangle = "#999999"
	const colorOutline  = "#ffffff"

	function render(d) {
		let dx = Math.cos(d)
		let dy = Math.sin(d)
		let t  = Math.tan(d)

		function roundTo(num, n) {
			n = Math.pow(10, n)
			return Math.round(num * n) / n
		}

		let dxDisplay = roundTo(dx, 3)
		let dyDisplay = roundTo(dy * -1, 3)
		let tDisplay  = roundTo(t, 3) * -1

		let angle
		if (unit == "Degrees") {
			angle = Math.round(180 / Math.PI * d * -1)
			if (angle < 0)
				angle += 360

			if (angle == 90 || angle == 270)
				tDisplay = "undefined"

			angle += "°"
		} else if (unit == "Radians") {
			angle = roundTo(d * -1, 3)

			if (angle == 1.571 || angle == -1.571)
				tDisplay = "undefined"
		}

		cosInfo.innerHTML = `cos(${angle}) = ${dxDisplay}`
		sinInfo.innerHTML = `sin(${angle}) = ${dyDisplay}`
		tanInfo.innerHTML = `tan(${angle}) = ${tDisplay}`

		ctx.clearRect(0, 0, w, h)

		ctx.beginPath()
		ctx.arc(r, r, circleR, 0, 2 * Math.PI, false)
		ctx.strokeStyle = colorCircle
		ctx.lineWidth   = 2
		ctx.stroke()

		ctx.beginPath()
		ctx.moveTo(r, 0)
		ctx.lineTo(r, h)
		ctx.moveTo(0, r)
		ctx.lineTo(w, r)
		ctx.strokeStyle = colorLines
		ctx.lineWidth   = 1
		ctx.stroke()

		ctx.beginPath()
		ctx.arc(r, r, 30, 0, d, true)
		ctx.strokeStyle = colorCircle
		ctx.lineWidth   = 1
		ctx.stroke()

		let x = circleR + dx * circleR + offset
		let y = circleR + dy * circleR + offset

		ctx.beginPath()
		ctx.moveTo(r, r)
		ctx.lineTo(x, y)
		ctx.strokeStyle = colorTriangle
		ctx.lineWidth   = 2
		ctx.stroke()

		ctx.beginPath()
		ctx.moveTo(x, y)
		ctx.lineTo(x, r)
		ctx.strokeStyle = colorSin
		ctx.stroke()

		ctx.font      = "15px Comic Sans MS";
		ctx.fillStyle = colorSin;
		ctx.textAlign = "center";
		ctx.fillText(dyDisplay, x, (y - r) / 2 + r + 6);

		ctx.beginPath()
		ctx.moveTo(r, r)
		ctx.lineTo(x, r)
		ctx.strokeStyle = colorCos
		ctx.stroke()

		ctx.font      = "15px Comic Sans MS";
		ctx.fillStyle = colorCos;
		ctx.textAlign = "center";
		ctx.fillText(dxDisplay, (x - r) / 2 + r, r - 5);

		if (tDisplay != "undefined") {
			dx = Math.cos(d + Math.PI / 2 * (t > 0? -1 : 1))
			dy = Math.sin(d + Math.PI / 2 * (t > 0? -1 : 1))

			ctx.beginPath()
			ctx.moveTo(x, y)
			ctx.lineTo(x + dx * w, y + dy * h)
			ctx.strokeStyle = colorTan
			ctx.stroke()

			ctx.font      = "15px Comic Sans MS";
			ctx.fillStyle = colorTan;
			ctx.textAlign = "center";
			ctx.fillText(tDisplay, x + dx * 40, y + dy * 40);
		}
	}

	render(Math.cos(Math.PI / 180 * 45), Math.sin(Math.PI / 180 * 45))

	function getMousePos(canvas, evt) {
		let rect = canvas.getBoundingClientRect()
		return {
			x: evt.clientX - rect.left,
			y: evt.clientY - rect.top,
		}
	}

	function update() {
		render(Math.atan2(mousePos.y - r, mousePos.x - r))
	}

	canvas.addEventListener("mousemove", function(evt) {
		mousePos = getMousePos(canvas, evt)
		update()
	}, false)
}

function sinCosImage() {
	let mousePos = {x: 0, y: 0}

	let canvas = document.getElementById("sin-cos-image")
	let ctx    = canvas.getContext("2d")

	const w = canvas.width, h = canvas.height
	const r = h / 2

	let offset = 10

	let circleW = h - offset * 2, circleH = h - offset * 2
	let circleR = r - offset

	const colorCircle   = "#c850c8"
	const colorLines    = "#999999"
	const colorSin      = "#357ff5"
	const colorCos      = "#00c800"
	const colorTan      = "#f8a800"
	const colorTriangle = "#999999"
	const colorOutline  = "#ffffff"

	function render(d) {
		let dx = Math.cos(d)
		let dy = Math.sin(d)
		let t  = Math.tan(d)

		function roundTo(num, n) {
			n = Math.pow(10, n)
			return Math.round(num * n) / n
		}

		ctx.clearRect(0, 0, w, h)

		ctx.beginPath()
		ctx.arc(r, r, circleR, 0, 2 * Math.PI, false)
		ctx.strokeStyle = colorCircle
		ctx.lineWidth   = 2
		ctx.stroke()

		ctx.beginPath()
		ctx.moveTo(r, 0)
		ctx.lineTo(r, h)
		ctx.moveTo(0, r)
		ctx.lineTo(w, r)
		ctx.strokeStyle = colorLines
		ctx.lineWidth   = 1
		ctx.rect(h, offset, w - h - offset, h - offset * 2)
		ctx.stroke()

		ctx.beginPath()
		ctx.arc(r, r, 20, 0, d, true)
		ctx.strokeStyle = colorCircle
		ctx.lineWidth   = 1
		ctx.stroke()

		let x = circleR + dx * circleR + offset
		let y = circleR + dy * circleR + offset

		ctx.beginPath()
		ctx.moveTo(r, r)
		ctx.lineTo(x, y)
		ctx.strokeStyle = colorTriangle
		ctx.lineWidth   = 2
		ctx.stroke()

		ctx.beginPath()
		ctx.moveTo(x, y)
		ctx.lineTo(x, r)
		ctx.strokeStyle = colorSin
		ctx.stroke()

		ctx.beginPath()
		ctx.moveTo(x, y)
		ctx.lineTo(h, y)
		ctx.lineWidth = 1
		ctx.stroke()

		ctx.beginPath()
		ctx.moveTo(r, r)
		ctx.lineTo(x, r)
		ctx.lineWidth   = 2
		ctx.strokeStyle = colorCos
		ctx.stroke()

		ctx.beginPath()
		ctx.moveTo(h, y)
		ctx.strokeStyle = colorSin

		for (let i = 0; i < w - h - offset; ++ i) {
			ctx.lineTo(h + i, h / 2 + Math.sin(d + i / 20) * (h / 2 - offset))
		}

		ctx.stroke()
	}

	render(Math.cos(Math.PI / 180 * 45), Math.sin(Math.PI / 180 * 45))

	function getMousePos(canvas, evt) {
		let rect = canvas.getBoundingClientRect()
		return {
			x: evt.clientX - rect.left,
			y: evt.clientY - rect.top,
		}
	}

	function update() {
		render(Math.atan2(mousePos.y - r, mousePos.x - r))
	}

	canvas.addEventListener("mousemove", function(evt) {
		mousePos = getMousePos(canvas, evt)
		update()
	}, false)
}

trigImage()
sinCosImage()
