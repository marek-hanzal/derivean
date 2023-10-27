import {
    type IInventoryService,
    withInventoryService
}                                    from "@derivean/inventory";
import {DateTime}                    from "@use-pico/i18n";
import {type IProducerService}       from "../api/IProducerService";
import {type ProducerProcessSchema}  from "../schema/ProducerProcessSchema";
import {type ProducerSnapshotSchema} from "../schema/ProducerSnapshotSchema";

export class ProducerService implements IProducerService {
    static inject = [
        withInventoryService.key,
    ];

    constructor(
        protected inventoryService: IInventoryService,
    ) {
    }

    public timeOf({producer: {input}}: ProducerProcessSchema.Type): number {
        return input.reduce((current, prev) => {
            return current + prev.time;
        }, 0);
    }

    public cycles(process: ProducerProcessSchema.Type): number {
        const cycleTime = Math.max(1, this.timeOf(process));
        return DateTime.fromISO(process.date.to).diff(DateTime.fromISO(process.date.from), "second").get("second") / cycleTime;
    }

    public inputAmountOf(process: ProducerProcessSchema.Type, name: string): number {
        return process
            .producer
            .input
            .filter(({resource}) => resource.name === name)
            .reduce((amount, resource) => {
                return amount + resource.amount;
            }, 0);
    }

    public outputAmountOf(process: ProducerProcessSchema.Type, name: string): number {
        return process
            .producer
            .output
            .filter(({resource}) => resource.name === name)
            .reduce((amount, resource) => {
                return amount + resource.amount;
            }, 0);
    }

    public process(process: ProducerProcessSchema.Type): ProducerSnapshotSchema.Type {
        const cycles = Math.floor(this.cycles(process));
        const resources = process.producer.input.map(({resource}) => resource.name);

        const maximums = [];
        for (let resource of process.producer.input) {
            const input = Math.max(1, this.inputAmountOf(process, resource.resource.name));
            const available = this.inventoryService.amountOf(process.inventory, resource.resource.name);

            maximums.push(available / input);
        }
        const maximum = Math.floor(Math.min(cycles, ...maximums));

        return {
            isLimit:   cycles >= Math.min(...maximums),
            inventory: this.inventoryService.normalizeOf({
                resources: [
                    ...process
                        .inventory
                        .resources
                        .filter(({resource}) => resources.includes(resource.name))
                        .map(resource => {
                            return {
                                ...resource,
                                amount: this.inputAmountOf(process, resource.resource.name) * maximum * -1,
                            };
                        }),
                    ...process.producer.output
                        .map(resource => {
                            return {
                                ...resource,
                                amount: this.outputAmountOf(process, resource.resource.name) * maximum,
                            };
                        })
                ],
            }),
        };
    }
}
