export {};

//
// export const withResourceMutation = withMutation({
//     key:         ["derivean", "resource", "mutation"],
//     schema:      {
//         request:  ResourceMutationSchema,
//         response: ResourceSchema,
//     },
//     mutator:     withResourceMutationAction,
//     invalidator: async () => [
//         withResourceQuery.key,
//         withResourceCountQuery.key
//     ],
// });
// export type withResourceMutation = typeof withResourceMutation;
