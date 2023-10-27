"use client";

import {
    ResourceTable,
    withResourceQuery
}                    from "@derivean/resource";
import {IconSubtask} from "@tabler/icons-react";
import {Page}        from "@use-pico/ui";

export default function Index() {
    const ResourceQueryProvider = withResourceQuery.query.Provider;

    return <Page
        icon={<IconSubtask/>}
        title={"manager.index"}
    >
        <ResourceQueryProvider>
            <ResourceTable/>
        </ResourceQueryProvider>
    </Page>;
}
