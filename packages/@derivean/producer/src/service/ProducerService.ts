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
        withInventoryService.inject,
    ];

    constructor(
        protected inventoryService: IInventoryService,
    ) {
    }

    public cycles(process: ProducerProcessSchema.Type): number {
        /**
         * Zero/minus values are considered as an instant pickup, thus generating one cycle
         */
        if (process.producer.time <= 0) {
            return 1;
        }
        return DateTime.fromISO(process.date.to).diff(DateTime.fromISO(process.date.from), "second").get("second") / process.producer.time;
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
        const resourceMaximum = Math.floor(Math.min(...maximums));
        const maximum = Math.floor(Math.min(cycles, resourceMaximum));

        return {
            isLimit:   cycles >= Math.min(...maximums),
            isReady: cycles > 0 && resourceMaximum > 0,
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
