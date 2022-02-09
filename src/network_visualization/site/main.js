

let network_data = {};
let current_selected_data = undefined
let change_to_data = undefined

let last_data = ""
let last_size = [0, 0]

let link_lookup = {
    loopback: { distance: 200, color: "#00ff0011", strength: 2 },
    unknown: { distance: 200, color: "#ffff00aa", strength: 2 },
    loopback_transport_route: { distance: 400, color: "#ff00ffaa", strength: .5 },
    socket_server_transport_route: { distance: 400, color: "#ffaa00aa", strength: .5 },
    socket_client_transport_route: { distance: 400, color: "#ffaa00aa", strength: .5 }
}


let drag_subject = undefined

let force_link = d3.forceLink().distance((d) => link_lookup[d.type].distance).strength((d) => link_lookup[d.type].strength)
let simulation = d3.forceSimulation()
    .force('charge', d3.forceManyBody().strength(-2000))
    .force('link', force_link)


function objectsAreSame(x, y) {
    if (x.length != y.length) return false;

    for (let i = 0; i < x.length; i++) {
        for (let prop in x[i])
            if (x[i][prop] != y[i][prop]) return false
    }
    return true;
}

function reset() {
    let width = document.querySelector('#content svg').width.baseVal.value
    let height = document.querySelector('#content svg').height.baseVal.value

    if (width == 0 && height == 0) {
        setTimeout(reset, 0.1)
        return
    }

    fetch("data").then((response) => {
        response.text().then((data) => {
            let changed = false

            if (data != last_data || change_to_data != current_selected_data) {
                changed = true
                current_selected_data = change_to_data

                let current_name = undefined

                for (let line of data.split('\n')) {
                    let chunks = line.split('$')
                    if (chunks[0] == ':') {

                        current_name = chunks[1];

                        if (current_selected_data == undefined) current_selected_data = change_to_data = current_name

                        network_data[current_name] = {
                            nodes: [],
                            new_nodes: [],
                            links: []
                        }

                    } else if (chunks[0] == '#') {

                        let node_id = parseInt(chunks[1])
                        network_data[current_name].new_nodes[node_id] = { eps: [] }

                        let idx = 2;
                        while (idx < chunks.length) {
                            network_data[current_name].new_nodes[node_id].eps.push({
                                address: chunks[idx++],
                                name: chunks[idx++]
                            })
                        }


                    } else if (chunks[0] == '~') {
                        network_data[current_name].links.push({
                            source: parseInt(chunks[1]),
                            target: parseInt(chunks[2]),
                            id: chunks[3],
                            type: chunks[4] || "unknown",
                            info: chunks[5] || "<empty>"
                        })
                    }
                }

                console.log(network_data);

                for (let name in network_data) {

                    let current_data = network_data[name]

                    for (let link of current_data.links) {
                        if (!link.offset) {
                            let id_links = current_data.links.filter((x) => x.target == link.target && x.source == link.source)
                            let total_connections_between_nodes = id_links.length - 1;
                            let offset_range_min = -total_connections_between_nodes / 2;
                            for (let i = 0; i < id_links.length; i++) {
                                id_links[i].offset = offset_range_min + i
                            }
                        }
                    }

                    current_data.nodes = current_data.new_nodes
                }


                simulation.nodes(network_data[current_selected_data].nodes)
                force_link.links(network_data[current_selected_data].links)


            }

            if (last_size[0] != width || last_size[1] != height) {
                changed = true
                simulation.force('center', d3.forceCenter(width / 2, height / 2))
            }

            if (changed) {
                console.log("reset");
                simulation.on('tick', ticked);
                simulation.alpha(0.5)
                simulation.restart()
            }


            last_data = data
            last_size = [width, height]
        })


    }).catch((e) => {
        console.error(e)

    })


}

function updateLinks() {
    d3.select('.links')
        .selectAll('line')
        .data(network_data[current_selected_data].links)
        .join('line').datum((data, index, svg_list) => {
            let offset = magTo(data.source.y - data.target.y, data.source.x - data.target.x, network_data[current_selected_data].links[index].offset * (8 + 4))

            let line = svg_list[index]
            line.setAttribute("x1", data.source.x + offset.x)
            line.setAttribute("x2", data.target.x + offset.x)
            line.setAttribute("y1", data.source.y - offset.y)
            line.setAttribute("y2", data.target.y - offset.y)
            line.setAttribute("stroke", link_lookup[data.type].color)

            line.innerHTML = `
                <title>${data.id} - ${data.type}</title>
            `
        })
}

function updateNodes() {
    d3.select('.nodes')
        .selectAll('.nodes>g')
        .data(network_data[current_selected_data].nodes)
        .join('g').call(d3.drag()
            .on("start", dragstart)
            .on("drag", dragged)
            .on("end", dragend))
        .datum((data, index, svg_list) => {
            let node = svg_list[index]

            let height = 40
            let font_size = height * 0.5



            let max_length = 0;
            for (let ep of data.eps) max_length = Math.max(max_length, ep.name.length)

            let text_height = font_size * 0.75
            let text_width = max_length * font_size * 0.6
            let width = text_width + height - text_height


            let padding = 5

            node.setAttribute("transform", `translate(${data.x - width / 2},${data.y - ((height + padding) * data.eps.length) / 2})`)
            node.setAttribute("index", index)

            let html = ""

            for (let i = 0; i < data.eps.length; i++) {
                html += `
                <g class="endpoint" transform="translate(0,${(height + padding) * i + padding / 2})">
                    <rect width="${width}" height="${height}" rx="${height / 2}" ry="${height / 2}"/>
                    <text font-size="${font_size}px" dx="${width / 2}" dy="${height / 2 + font_size * 0.35}" >${data.eps[i].name}</text>
                </g>
                `
            }

            node.innerHTML = html
        })
}

function ticked() {
    updateLinks()
    updateNodes()
}


function getNearest(event) {
    let subject;
    let dist = Infinity;

    for (let node of network_data[current_selected_data].nodes) {
        let d = Math.pow(node.x - event.subject.x, 2) + Math.pow(node.y - event.subject.y, 2)
        if (d < dist) {
            subject = node;
            dist = d;
        }
    }

    return subject
}

function dragstart(event) {
    drag_subject = getNearest(event)

    simulation.alphaTarget(0.3).restart()

    drag_subject.fx = null;
    drag_subject.fy = null;
}

function dragged(event) {
    drag_subject.fx = event.x;
    drag_subject.fy = event.y;
}

function dragend(event) {
    simulation.alphaTarget(0)
}




function magTo(x, y, mag) {

    let m = Math.sqrt(x * x + y * y) / mag
    return { x: x / m, y: y / m }
}

window.onresize = reset
setInterval(reset, 1000)
