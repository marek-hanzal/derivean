import {
    withInventoryContainer,
    withInventoryService
}                              from "@derivean/inventory";
import {ResourceSchema}        from "@derivean/resource";
import {Container}             from "@use-pico/container";
import {DateTime}              from "@use-pico/i18n";
import {
    describe,
    expect,
    test
}                              from "vitest";
import {
    ProducerSchema,
    withProducerContainer,
    withProducerService
}                              from "../src";
import {ProducerProcessSchema} from "../src/schema/ProducerProcessSchema";

const TreeResource: ResourceSchema.Type = {
    name: "tree",
};
const LogResource: ResourceSchema.Type = {
    name: "log",
};
const SawdustResource: ResourceSchema.Type = {
    name: "sawdust",
};

const producer: ProducerSchema.Type = {
    input:  [
        {
            resource: TreeResource,
            time:     10,
            amount:   1,
        },
    ],
    output: [
        {
            resource: LogResource,
            amount:   6,
            time:     15,
        },
        {
            resource: SawdustResource,
            amount:   100,
            time:     7,
        },
    ],
};
const date = {
    from: DateTime.now().minus({
        second: 65,
    }).toISO(),
    to:   DateTime.now().toISO(),
};

const goodProducerProcess: ProducerProcessSchema.Type = {
    date,
    producer,
    inventory: {
        resources: [
            {
                resource: TreeResource,
                amount:   10,
            },
            {
                resource: LogResource,
                amount:   2,
            },
        ],
    },
};
const notEnoughResourceProducerProcess: ProducerProcessSchema.Type = {
    date,
    producer,
    inventory: {
        resources: [
            {
                resource: TreeResource,
                amount:   0,
            },
            {
                resource: LogResource,
                amount:   2,
            },
        ],
    },
};
const missingResourceProducerProcess: ProducerProcessSchema.Type = {
    date,
    producer,
    inventory: {
        resources: [
            {
                resource: LogResource,
                amount:   2,
            },
        ],
    },
};

describe("ProducerService", () => {
    const container = new Container();
    withProducerContainer(container);
    withInventoryContainer(container);
    const producerService = withProducerService.resolve(container);
    const inventoryService = withInventoryService.resolve(container);

    test("Input/output times works", () => {
        expect(producerService.inputTime(goodProducerProcess)).toBe(10);
        expect(producerService.outputTime(goodProducerProcess)).toBe(15);
    });

    test("Number of cycles", () => {
        expect(Math.floor(producerService.cycles(goodProducerProcess))).toBe(2);
    });

    test("Production works", () => {
        const snapshot = producerService.process(goodProducerProcess);

        expect(snapshot.isLimit).toBeFalsy();

        const treeResourceRelative = inventoryService.resourceOf(snapshot.relative, TreeResource.name);
        const treeResource = inventoryService.resourceOf(snapshot.inventory, TreeResource.name);

        const logResourceRelative = inventoryService.resourceOf(snapshot.relative, LogResource.name);
        const logResource = inventoryService.resourceOf(snapshot.inventory, LogResource.name);

        const sawdustResourceRelative = inventoryService.resourceOf(snapshot.relative, SawdustResource.name);
        const sawdustResource = inventoryService.resourceOf(snapshot.inventory, SawdustResource.name);

        expect(treeResourceRelative.length).toBe(1);
        expect(treeResource.length).toBe(1);

        expect(logResourceRelative.length).toBe(1);
        expect(logResource.length).toBe(1);

        expect(sawdustResourceRelative.length).toBe(1);
        expect(sawdustResource.length).toBe(1);

        expect(treeResourceRelative[0].amount).toBe(-2);
        expect(logResourceRelative[0].amount).toBe(12);
        expect(sawdustResourceRelative[0].amount).toBe(200);

        expect(treeResource[0].amount).toBe(8);
        expect(logResource[0].amount).toBe(14);
        expect(sawdustResource[0].amount).toBe(200);
    });

    test("Production missing resources", () => {
        const snapshot = producerService.process(missingResourceProducerProcess);

        const treeResourceRelative = inventoryService.resourceOf(snapshot.relative, TreeResource.name);
        const treeResource = inventoryService.resourceOf(snapshot.inventory, TreeResource.name);

        const logResourceRelative = inventoryService.resourceOf(snapshot.relative, LogResource.name);
        const logResource = inventoryService.resourceOf(snapshot.inventory, LogResource.name);

        const sawdustResourceRelative = inventoryService.resourceOf(snapshot.relative, SawdustResource.name);
        const sawdustResource = inventoryService.resourceOf(snapshot.inventory, SawdustResource.name);

        expect(treeResourceRelative.length).toBe(0);
        expect(treeResource.length).toBe(0);

        expect(logResourceRelative.length).toBe(1);
        expect(logResource.length).toBe(1);

        expect(sawdustResourceRelative.length).toBe(1);
        expect(sawdustResource.length).toBe(1);

        expect(logResourceRelative[0].amount).toBe(0);
        expect(sawdustResourceRelative[0].amount).toBe(0);

        expect(logResource[0].amount).toBe(2);
        expect(sawdustResource[0].amount).toBe(0);
    });

    test("Production missing resources - is limit", () => {
        const snapshot = producerService.process(missingResourceProducerProcess);
        expect(snapshot.isLimit).toBeTruthy();
    });

    test("Production not enough resources", () => {
        const snapshot = producerService.process(notEnoughResourceProducerProcess);

        const treeResourceRelative = inventoryService.resourceOf(snapshot.relative, TreeResource.name);
        const treeResource = inventoryService.resourceOf(snapshot.inventory, TreeResource.name);

        const logResourceRelative = inventoryService.resourceOf(snapshot.relative, LogResource.name);
        const logResource = inventoryService.resourceOf(snapshot.inventory, LogResource.name);

        const sawdustResourceRelative = inventoryService.resourceOf(snapshot.relative, SawdustResource.name);
        const sawdustResource = inventoryService.resourceOf(snapshot.inventory, SawdustResource.name);

        expect(treeResourceRelative.length).toBe(1);
        expect(treeResource.length).toBe(1);

        expect(logResourceRelative.length).toBe(1);
        expect(logResource.length).toBe(1);

        expect(sawdustResourceRelative.length).toBe(1);
        expect(sawdustResource.length).toBe(1);

        expect(logResourceRelative[0].amount).toBe(0);
        expect(sawdustResourceRelative[0].amount).toBe(0);

        expect(logResource[0].amount).toBe(2);
        expect(sawdustResource[0].amount).toBe(0);
    });

    test("Production not enough resources - is limit", () => {
        const snapshot = producerService.process(notEnoughResourceProducerProcess);
        expect(snapshot.isLimit).toBeTruthy();
    });
});
