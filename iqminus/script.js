let buttonBackToTop = document.getElementById("back-to-top")
let siteNav         = document.getElementById("site-nav")

let buttonDarkTheme  = document.getElementById("dark-theme")
let buttonLightTheme = document.getElementById("light-theme")

function getCookie(name) {
	let value = `; ${document.cookie}`
	let parts = value.split(`; ${name}=`)
	if (parts.length === 2)
		return parts.pop().split(';').shift()
}

function backToTop() {
	window.scrollTo({
		top:      0,
		behavior: "smooth",
	});
}

function setTheme(name) {
	switch (name) {
	case "light":
		document.cookie = "theme=light; path=/"
		document.documentElement.className = "light"
		break

	case "dark":
		document.cookie = "theme=dark; path=/"
		document.documentElement.className = "dark"
		break
	}
}

function closeMenu() {
	siteNav.classList.remove("open")
}

function openMenu() {
	siteNav.classList.add("open")
}

window.onscroll = () => {
	let scrollTop = document.body.scrollTop || document.documentElement.scrollTop

	if (scrollTop > 20)
		buttonBackToTop.style.opacity = 1
	else
		buttonBackToTop.style.opacity = 0
}

let theme = getCookie("theme")
if (theme)
	setTheme(theme)
else
	setTheme("light")
