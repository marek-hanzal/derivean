import {
    type IInventoryService,
    withInventoryService
}                                     from "@derivean/inventory";
import {type GraphSchema}             from "@use-pico/diagram";
import {withDullSchema}               from "@use-pico/dull-stuff";
import {DateTime}                     from "@use-pico/i18n";
import {uniqueOf}                     from "@use-pico/utils";
import {type IProducerProcess}        from "../api/IProducerProcess";
import {type IProducerService}        from "../api/IProducerService";
import {type IProducerSnapshot}       from "../api/IProducerSnapshot";
import {withProducerInputRepository}  from "../container/withProducerInputRepository";
import {withProducerOutputRepository} from "../container/withProducerOutputRepository";
import {withProducerRepository}       from "../container/withProducerRepository";
import {DependencyError}              from "../error/DependencyError";
import {ProducerInputRepository}      from "../repository/ProducerInputRepository";
import {ProducerOutputRepository}     from "../repository/ProducerOutputRepository";
import {ProducerRepository}           from "../repository/ProducerRepository";
import {ProducerSchema}               from "../schema/ProducerSchema";
import {ProductionTimeSchema}         from "../schema/ProductionTimeSchema";

export class ProducerService implements IProducerService {
    static inject = [
        withInventoryService.inject,
        withProducerRepository.inject,
        withProducerInputRepository.inject,
        withProducerOutputRepository.inject,
    ];

    constructor(
        protected inventoryService: IInventoryService,
        protected producerRepository: ProducerRepository.Type,
        protected producerInputRepository: ProducerInputRepository.Type,
        protected producerOutputRepository: ProducerOutputRepository.Type,
    ) {
    }

    public cycles(process: IProducerProcess): number {
        /**
         * Zero/minus values are considered as an instant pickup, thus generating one cycle
         */
        if (process.producer.time <= 0) {
            return 1;
        }
        return DateTime.fromISO(process.date.to).diff(DateTime.fromISO(process.date.from), "second").get("second") / process.producer.time;
    }

    public inputAmountOf(process: IProducerProcess, name: string): number {
        return process
            .producer
            .input
            .filter(({resource}) => resource.name === name)
            .reduce((amount, resource) => {
                return amount + resource.amount;
            }, 0);
    }

    public outputAmountOf(process: IProducerProcess, name: string): number {
        return process
            .producer
            .output
            .filter(({resource}) => resource.name === name)
            .reduce((amount, resource) => {
                return amount + resource.amount;
            }, 0);
    }

    public process(process: IProducerProcess): IProducerSnapshot {
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

    public async dependencies(producerId: string, stack: string[] = []): Promise<withDullSchema.Infer.Entity<ProducerSchema>[]> {
        const dependencies = [];
        const producers = this.producerRepository.withQuery.query({
            where: {
                idIn: (await this.producerOutputRepository
                    .withQuery
                    .select(["ProducerOutput.producerId"])
                    .where(
                        "resourceId",
                        "in",
                        this.producerInputRepository
                            .withQuery
                            .select(["ProducerInput.resourceId"])
                            .where("producerId", "=", producerId)
                    )
                    .execute())
                          .map(({producerId}) => producerId)
            }
        });

        for (const producer of await producers) {
            if (stack?.includes(producer.id)) {
                throw new DependencyError(
                    `Infinite loop detected: ${stack.join(" -> ")} -> ${producer.id}`,
                    await this.producerRepository.withQuery.query({
                        where: {
                            idIn: stack
                        }
                    })
                );
            }

            dependencies.push(
                producer,
                ...await this.dependencies(
                    producer.id,
                    [...stack, producer.id]
                )
            );
        }

        return uniqueOf(dependencies, "id");
    }

    public async timeOf(producerId: string): Promise<ProductionTimeSchema.Type> {
        let time = 0;

        const producer = await this.producerRepository.withQuery.fetchOrThrow({
            where: {
                id: producerId,
            }
        });

        for (const producer of await this.dependencies(producerId)) {
            time += producer.time;
        }

        return {
            time: producer.time + time,
        };
    }

    public async graph(producerId: string): Promise<GraphSchema.Type> {
        const dependencies = (await this.dependencies(producerId))
            .concat(
                await this.producerRepository.withQuery.fetchOrThrow({
                    where: {id: producerId},
                })
            );
        const edges: GraphSchema.Type["edges"] = [];

        for (const producer of dependencies) {
            for (const input of await this.producerInputRepository.withQuery.select(["ProducerInput.resourceId"]).where("producerId", "=", producer.id).execute()) {
                /**
                 * @TODO fix select() method, add typing, so it would accept all tables
                 */
                for (const output of await this.producerOutputRepository.withQuery.select(["ProducerOutput.producerId", "Resource.name" as any, "ProducerOutput.amount" as any]).innerJoin("Resource", "Resource.id", "ProducerOutput.resourceId" as any).where("resourceId", "=", input.resourceId).execute()) {
                    edges.push({
                        to:    producer.id,
                        from:  output.producerId,
                        label: `${output.name} (x${output.amount})`,
                    });
                }
            }
        }

        return {
            nodes: dependencies.map(producer => ({
                id:    producer.id,
                label: producer.name,
                color: producer.id === producerId ? "#FFF" : "#DDD",
            })),
            edges,
        };
    }
}
