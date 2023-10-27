import {ResourceMutationRpc} from "@derivean/resource";
import {IconSubtask}         from "@tabler/icons-react";
import {
    Button,
    Page
}                            from "@use-pico/ui";

export default function Index() {
    const mut = ResourceMutationRpc.useMutation();

    return <Page
        icon={<IconSubtask/>}
        title={"manager.index"}
    >
        <Button
            onClick={() => {
                mut.mutate({
                    create: {
                        name: "foo",
                    },
                });
            }}
        >
            foo
        </Button>
    </Page>;
}
