"use client";

import {ResourceMutationRpcHandler} from "@derivean/resource";
import {IconSubtask}                from "@tabler/icons-react";
import {useRpcMutation}             from "@use-pico/rpc";
import {
    Button,
    Page
}                                   from "@use-pico/ui";

export default function Index() {
    const mut = useRpcMutation(ResourceMutationRpcHandler);

    return <Page
        icon={<IconSubtask/>}
        title={"manager.index"}
    >
        <Button
            onClick={() => {
                mut.mutate({
                    create: {
                        name2: "foo",
                    },
                }, {
                    onSuccess: data => {
                        data.fdf;
                    }
                });
            }}
        >
            foo
        </Button>
    </Page>;
}
