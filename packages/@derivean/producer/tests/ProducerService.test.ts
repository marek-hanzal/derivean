import {
    withInventoryContainer,
    withInventoryService
}                       from "@derivean/inventory";
import {type IResource} from "@derivean/resource";
import {Container}      from "@use-pico/container";
import {DateTime}       from "@use-pico/i18n";
import {
    describe,
    expect,
    test
}                       from "vitest";
import {
    type IProducer,
    type IProducerProcess,
    withProducerContainer,
    withProducerService
}                       from "../src";

const TreeResource: IResource = {
    name: "tree",
};
const LeafsResource: IResource = {
    name: "leafs",
};
const LogResource: IResource = {
    name: "log",
};
const SawdustResource: IResource = {
    name: "sawdust",
};

const producer: IProducer = {
    time:   10,
    input:  [
        {
            resource: TreeResource,
            amount: 2,
        },
        {
            resource: LeafsResource,
            amount:   2,
        },
    ],
    output: [
        {
            resource: LogResource,
            amount:   6,
        },
        {
            resource: SawdustResource,
            amount:   100,
        },
    ],
};
const date = {
    from: DateTime.now().minus({
        second: 35,
    }).toISO(),
    to:   DateTime.now().toISO(),
};

const goodProducerProcess: IProducerProcess = {
    date,
    producer,
    inventory: {
        resources: [
            {
                resource: TreeResource,
                amount:   10,
            },
            {
                resource: LeafsResource,
                amount:   10,
            },
            {
                resource: LogResource,
                amount:   2,
            },
        ],
    },
};
const notEnoughResourceProducerProcess: IProducerProcess = {
    date,
    producer,
    inventory: {
        resources: [
            {
                resource: TreeResource,
                amount: 2,
            },
            {
                resource: LeafsResource,
                amount:   1,
            },
            {
                resource: LogResource,
                amount:   2,
            },
        ],
    },
};
const missingResourceProducerProcess: IProducerProcess = {
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
    const producerService = withProducerService.use(container);
    const inventoryService = withInventoryService.use(container);

    test("Number of cycles", () => {
        expect(Math.floor(producerService.cycles(goodProducerProcess))).toBe(3);
    });

    test("Production works", () => {
        const snapshot = producerService.process(goodProducerProcess);
        expect(snapshot.isLimit).toBeFalsy();
        expect(snapshot.isReady).toBeTruthy();

        const treeResource = inventoryService.resourceOf(snapshot.inventory, TreeResource.name);
        const logResource = inventoryService.resourceOf(snapshot.inventory, LogResource.name);
        const sawdustResource = inventoryService.resourceOf(snapshot.inventory, SawdustResource.name);

        expect(treeResource.length).toBe(1);
        expect(logResource.length).toBe(1);
        expect(sawdustResource.length).toBe(1);

        expect(treeResource[0].amount).toBe(-6);
        expect(logResource[0].amount).toBe(18);
        expect(sawdustResource[0].amount).toBe(300);
    });

    test("Production not ready", () => {
        const snapshot = producerService.process({
            ...goodProducerProcess,
            producer: {
                ...goodProducerProcess.producer,
                time: 6000,
            }
        });
        expect(snapshot.isReady).toBeFalsy();
    });

    test("Production missing resources", () => {
        const snapshot = producerService.process(missingResourceProducerProcess);

        const treeResource = inventoryService.resourceOf(snapshot.inventory, TreeResource.name);
        const logResource = inventoryService.resourceOf(snapshot.inventory, LogResource.name);
        const sawdustResource = inventoryService.resourceOf(snapshot.inventory, SawdustResource.name);

        expect(treeResource.length).toBe(0);
        expect(logResource.length).toBe(1);
        expect(sawdustResource.length).toBe(1);

        expect(logResource[0].amount).toBe(0);
        expect(sawdustResource[0].amount).toBe(0);
    });

    test("Production missing resources - is limit", () => {
        const snapshot = producerService.process(missingResourceProducerProcess);
        expect(snapshot.isLimit).toBeTruthy();
        expect(snapshot.isReady).toBeFalsy();
    });

    test("Production not enough resources", () => {
        const snapshot = producerService.process(notEnoughResourceProducerProcess);
        expect(snapshot.isLimit).toBeTruthy();
        expect(snapshot.isReady).toBeFalsy();

        const treeResource = inventoryService.resourceOf(snapshot.inventory, TreeResource.name);
        const logResource = inventoryService.resourceOf(snapshot.inventory, LogResource.name);
        const sawdustResource = inventoryService.resourceOf(snapshot.inventory, SawdustResource.name);

        expect(treeResource.length).toBe(1);
        expect(logResource.length).toBe(1);
        expect(sawdustResource.length).toBe(1);

        expect(logResource[0].amount).toBe(0);
        expect(sawdustResource[0].amount).toBe(0);
    });
});
