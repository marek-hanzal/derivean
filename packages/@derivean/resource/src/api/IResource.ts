export interface IResource {
    /**
     * Name of a resource (like log, gem, gold or even more abstract things).
     */
    name: string;
    /**
     * Type of resource; could be resource, building, item, ...
     */
    type: string;
}
