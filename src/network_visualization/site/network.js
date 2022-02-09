class Network {

    name = undefined
    network_data = null

    link_lookup = {
        loopback: { distance: 200, color: "#00ff0011", strength: 2 },
        unknown: { distance: 200, color: "#ffff00aa", strength: 2 },
        loopback_transport_route: { distance: 400, color: "#ff00ffaa", strength: .5 },
        socket_server_transport_route: { distance: 400, color: "#ffaa00aa", strength: .5 },
        socket_client_transport_route: { distance: 400, color: "#ffaa00aa", strength: .5 }
    }

    force_link = null
    simulation = null


    constructor(name) {
        this.name = name;

        this.force_link = d3.forceLink()
            .distance((d) => this.lookup_for_type(d.typestring).distance)
            .strength((d) => this.lookup_for_type(d.typestring).strength)

        this.simulation = d3.forceSimulation()
            .force('charge', d3.forceManyBody().strength(-2000))
            .force('link', this.force_link)


        let container = document.querySelector('#content')

        container.innerHTML += `
            <div class="svg-wrapper">
                <span class="svg-label">${this.name}</span>
                <svg id="${this.name}">
                    <g class="links"></g>
                    <g class="nodes"></g>
                </svg>
            </div>
        `

    }

    set_data(data) {
        this.network_data = data

        this.simulation.nodes(this.network_data.nodes)
        this.force_link.links(this.network_data.routes)

        this.simulation.on('tick', () => { this.updateLinks(); this.updateNodes() });
        this.resize()
    }

    resize() {
        let width = document.querySelector('#' + this.name).width.baseVal.value
        let height = document.querySelector('#' + this.name).height.baseVal.value
        this.simulation.force('center', d3.forceCenter(width / 2, height / 2))

        this.update()
    }

    update() {
        this.simulation.alpha(0.5)
        this.simulation.restart()
    }

    lookup_for_type(type) {
        return type ? this.link_lookup[type] : this.link_lookup['unknown']
    }

    magTo(x, y, mag) {
        let m = Math.sqrt(x * x + y * y) / mag
        return { x: x / m, y: y / m }
    }

    updateLinks() {
        d3.select(`#${this.name} .links`)
            .selectAll('line')
            .data(this.network_data.routes)
            .join('line').datum((data, index, svg_list) => {
                let mag = this.network_data.routes[index].offset * (8 + 4)
                let offset = this.magTo(data.source.y - data.target.y, data.source.x - data.target.x, mag)

                let line = svg_list[index]
                line.setAttribute("x1", data.source.x + offset.x)
                line.setAttribute("x2", data.target.x + offset.x)
                line.setAttribute("y1", data.source.y - offset.y)
                line.setAttribute("y2", data.target.y - offset.y)
                line.setAttribute("stroke", this.lookup_for_type(data.typestring).color)

                line.innerHTML = `
                    <title>${data.id} - ${data.type}</title>
                `
            })
    }

    updateNodes() {
        d3.select(`#${this.name} .nodes`)
            .selectAll('.nodes>g')
            .data(this.network_data.nodes)
            .join('g').call(d3.drag()
                .on("start", dragstart)
                .on("drag", dragged)
                .on("end", dragend))
            .datum((data, index, svg_list) => {
                let node = svg_list[index]

                let height = 40
                let font_size = height * 0.5

                let max_length = 0;
                for (let ep of data.endpoints) max_length = Math.max(max_length, ep.name.length)

                let text_height = font_size * 0.75
                let text_width = max_length * font_size * 0.6
                let width = text_width + height - text_height

                let padding = 5

                node.setAttribute("transform", `translate(${data.x - width / 2},${data.y - ((height + padding) * data.endpoints.length) / 2})`)
                node.setAttribute("index", index)

                let html = ""

                for (let i = 0; i < data.endpoints.length; i++) {
                    html += `
                    <g class="endpoint" transform="translate(0,${(height + padding) * i + padding / 2})">
                        <rect width="${width}" height="${height}" rx="${height / 2}" ry="${height / 2}"/>
                        <text font-size="${font_size}px" dx="${width / 2}" dy="${height / 2 + font_size * 0.35}" >${data.endpoints[i].name}</text>
                    </g>
                    `
                }

                node.innerHTML = html
            })
    }


}